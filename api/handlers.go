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

func loadEntries(path, pwd string) ([]Entry, error) {
	data, err := LoadVault(path, pwd)
	if err != nil {
		return nil, err
	}
	var entries []Entry
	if len(data) > 0 && string(data) != "{}" {
		if err := json.Unmarshal(data, &entries); err != nil {
			return nil, err
		}
	}
	if entries == nil {
		entries = []Entry{}
	}
	return entries, nil
}

func saveEntries(path, pwd string, entries []Entry) error {
	data, err := json.Marshal(entries)
	if err != nil {
		return err
	}
	return SaveVault(path, pwd, data)
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

func HandleListEntries() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			writeError(w, http.StatusMethodNotAllowed, "method not allowed")
			return
		}
		var req VaultRequest
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			writeError(w, http.StatusBadRequest, "invalid request")
			return
		}

		entries, err := loadEntries(req.VaultPath, req.MasterPassword)
		if err != nil {
			entries = []Entry{}
		}

		writeJSON(w, http.StatusOK, EntryListResponse{
			Entries: entries,
			Count:   len(entries),
		})
	}
}

func HandleCreateEntry() http.HandlerFunc {
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

		if req.Name == "" || req.VaultPath == "" || req.MasterPassword == "" {
			writeError(w, http.StatusBadRequest, "missing required fields")
			return
		}

		entries, _ := loadEntries(req.VaultPath, req.MasterPassword)

		entry := Entry{
			ID:       formatID(len(entries) + 1),
			Name:     req.Name,
			Username: req.Username,
			Password: req.Password,
			URL:      req.URL,
			Notes:    req.Notes,
		}
		entries = append(entries, entry)

		if err := saveEntries(req.VaultPath, req.MasterPassword, entries); err != nil {
			writeError(w, http.StatusInternalServerError, "failed to save vault")
			return
		}

		writeJSON(w, http.StatusCreated, entry)
	}
}

func HandleDeleteEntry() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			writeError(w, http.StatusMethodNotAllowed, "method not allowed")
			return
		}

		var req struct {
			VaultRequest
			ID string `json:"id"`
		}
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			writeError(w, http.StatusBadRequest, "invalid request")
			return
		}

		entries, err := loadEntries(req.VaultPath, req.MasterPassword)
		if err != nil {
			writeError(w, http.StatusInternalServerError, "failed to load vault")
			return
		}

		found := false
		var newEntries []Entry
		for _, e := range entries {
			if e.ID == req.ID {
				found = true
			} else {
				newEntries = append(newEntries, e)
			}
		}

		if !found {
			writeError(w, http.StatusNotFound, "entry not found")
			return
		}

		if err := saveEntries(req.VaultPath, req.MasterPassword, newEntries); err != nil {
			writeError(w, http.StatusInternalServerError, "failed to save vault")
			return
		}

		writeJSON(w, http.StatusOK, map[string]string{"status": "deleted"})
	}
}
