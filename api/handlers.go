package api

import (
	"encoding/json"
	"net/http"
	"sync"
)

type VaultStore struct {
	mu      sync.RWMutex
	entries map[string]Entry
	counter int
}

func NewVaultStore() *VaultStore {
	return &VaultStore{
		entries: make(map[string]Entry),
	}
}

func (vs *VaultStore) Add(e Entry) Entry {
	vs.mu.Lock()
	defer vs.mu.Unlock()
	vs.counter++
	e.ID = formatID(vs.counter)
	vs.entries[e.ID] = e
	return e
}

func (vs *VaultStore) Get(id string) (Entry, bool) {
	vs.mu.RLock()
	defer vs.mu.RUnlock()
	e, ok := vs.entries[id]
	return e, ok
}

func (vs *VaultStore) List() []Entry {
	vs.mu.RLock()
	defer vs.mu.RUnlock()
	result := make([]Entry, 0, len(vs.entries))
	for _, e := range vs.entries {
		result = append(result, e)
	}
	return result
}

func (vs *VaultStore) Delete(id string) bool {
	vs.mu.Lock()
	defer vs.mu.Unlock()
	if _, ok := vs.entries[id]; !ok {
		return false
	}
	delete(vs.entries, id)
	return true
}

func formatID(n int) string {
	return "entry-" + itoa(n)
}

func itoa(n int) string {
	if n == 0 {
		return "0"
	}
	digits := []byte{}
	for n > 0 {
		digits = append([]byte{byte('0' + n%10)}, digits...)
		n /= 10
	}
	return string(digits)
}

func writeJSON(w http.ResponseWriter, status int, v any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(v)
}

func writeError(w http.ResponseWriter, status int, msg string) {
	writeJSON(w, status, ErrorResponse{
		Error: msg,
		Code:  status,
	})
}

func HandleHealth() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			writeError(w, http.StatusMethodNotAllowed, "method not allowed")
			return
		}
		writeJSON(w, http.StatusOK, HealthResponse{
			Status:  "ok",
			Version: "0.1.0",
		})
	}
}

func HandleListEntries(store *VaultStore) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			writeError(w, http.StatusMethodNotAllowed, "method not allowed")
			return
		}
		entries := store.List()
		writeJSON(w, http.StatusOK, EntryListResponse{
			Entries: entries,
			Count:   len(entries),
		})
	}
}

func HandleCreateEntry(store *VaultStore) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			writeError(w, http.StatusMethodNotAllowed, "method not allowed")
			return
		}

		var req EntryRequest
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			writeError(w, http.StatusBadRequest, "invalid request body")
			return
		}

		if req.Name == "" {
			writeError(w, http.StatusBadRequest, "name is required")
			return
		}

		entry := store.Add(Entry{
			Name:     req.Name,
			Username: req.Username,
			Password: req.Password,
			URL:      req.URL,
			Notes:    req.Notes,
		})

		writeJSON(w, http.StatusCreated, entry)
	}
}

func HandleGetEntry(store *VaultStore) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			writeError(w, http.StatusMethodNotAllowed, "method not allowed")
			return
		}

		id := r.URL.Query().Get("id")
		if id == "" {
			writeError(w, http.StatusBadRequest, "id is required")
			return
		}

		entry, ok := store.Get(id)
		if !ok {
			writeError(w, http.StatusNotFound, "entry not found")
			return
		}

		writeJSON(w, http.StatusOK, entry)
	}
}

func HandleDeleteEntry(store *VaultStore) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodDelete {
			writeError(w, http.StatusMethodNotAllowed, "method not allowed")
			return
		}

		id := r.URL.Query().Get("id")
		if id == "" {
			writeError(w, http.StatusBadRequest, "id is required")
			return
		}

		if !store.Delete(id) {
			writeError(w, http.StatusNotFound, "entry not found")
			return
		}

		writeJSON(w, http.StatusOK, map[string]string{"status": "deleted"})
	}
}
