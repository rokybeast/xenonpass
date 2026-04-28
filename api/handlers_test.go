package api

import (
	"bytes"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"
)

func TestHandleHealth_Success(t *testing.T) {
	handler := HandleHealth()
	req := httptest.NewRequest(http.MethodGet, "/api/health", nil)
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusOK {
		t.Fatalf("expected 200, got %d", rr.Code)
	}

	var resp HealthResponse
	json.NewDecoder(rr.Body).Decode(&resp)

	if resp.Status != "ok" {
		t.Errorf("expected status ok, got %q", resp.Status)
	}
	if resp.Version != "0.1.0" {
		t.Errorf("expected version 0.1.0, got %q", resp.Version)
	}
}

func TestHandleHealth_WrongMethod(t *testing.T) {
	handler := HandleHealth()
	req := httptest.NewRequest(http.MethodPost, "/api/health", nil)
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusMethodNotAllowed {
		t.Fatalf("expected 405, got %d", rr.Code)
	}
}

func TestHandleCreateEntry_Success(t *testing.T) {
	store := NewVaultStore()
	handler := HandleCreateEntry(store)

	body, _ := json.Marshal(EntryRequest{
		Name:     "GitHub",
		Username: "user@example.com",
		Password: "secret123",
		URL:      "https://github.com",
	})

	req := httptest.NewRequest(http.MethodPost, "/api/entries/create", bytes.NewReader(body))
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusCreated {
		t.Fatalf("expected 201, got %d", rr.Code)
	}

	var entry Entry
	json.NewDecoder(rr.Body).Decode(&entry)

	if entry.ID == "" {
		t.Error("expected non-empty ID")
	}
	if entry.Name != "GitHub" {
		t.Errorf("expected name GitHub, got %q", entry.Name)
	}
	if entry.Username != "user@example.com" {
		t.Errorf("expected username user@example.com, got %q", entry.Username)
	}
}

func TestHandleCreateEntry_MissingName(t *testing.T) {
	store := NewVaultStore()
	handler := HandleCreateEntry(store)

	body, _ := json.Marshal(EntryRequest{
		Username: "user",
		Password: "pass",
	})

	req := httptest.NewRequest(http.MethodPost, "/api/entries/create", bytes.NewReader(body))
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusBadRequest {
		t.Fatalf("expected 400, got %d", rr.Code)
	}
}

func TestHandleCreateEntry_InvalidJSON(t *testing.T) {
	store := NewVaultStore()
	handler := HandleCreateEntry(store)

	req := httptest.NewRequest(http.MethodPost, "/api/entries/create", bytes.NewReader([]byte("not json")))
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusBadRequest {
		t.Fatalf("expected 400, got %d", rr.Code)
	}
}

func TestHandleCreateEntry_WrongMethod(t *testing.T) {
	store := NewVaultStore()
	handler := HandleCreateEntry(store)

	req := httptest.NewRequest(http.MethodGet, "/api/entries/create", nil)
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusMethodNotAllowed {
		t.Fatalf("expected 405, got %d", rr.Code)
	}
}

func TestHandleListEntries_Empty(t *testing.T) {
	store := NewVaultStore()
	handler := HandleListEntries(store)

	req := httptest.NewRequest(http.MethodGet, "/api/entries", nil)
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusOK {
		t.Fatalf("expected 200, got %d", rr.Code)
	}

	var resp EntryListResponse
	json.NewDecoder(rr.Body).Decode(&resp)

	if resp.Count != 0 {
		t.Errorf("expected 0 entries, got %d", resp.Count)
	}
}

func TestHandleListEntries_WithEntries(t *testing.T) {
	store := NewVaultStore()
	store.Add(Entry{Name: "Entry1"})
	store.Add(Entry{Name: "Entry2"})

	handler := HandleListEntries(store)

	req := httptest.NewRequest(http.MethodGet, "/api/entries", nil)
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	var resp EntryListResponse
	json.NewDecoder(rr.Body).Decode(&resp)

	if resp.Count != 2 {
		t.Errorf("expected 2 entries, got %d", resp.Count)
	}
}

func TestHandleGetEntry_Success(t *testing.T) {
	store := NewVaultStore()
	added := store.Add(Entry{Name: "TestEntry", Username: "testuser"})

	handler := HandleGetEntry(store)

	req := httptest.NewRequest(http.MethodGet, "/api/entries/get?id="+added.ID, nil)
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusOK {
		t.Fatalf("expected 200, got %d", rr.Code)
	}

	var entry Entry
	json.NewDecoder(rr.Body).Decode(&entry)

	if entry.Name != "TestEntry" {
		t.Errorf("expected name TestEntry, got %q", entry.Name)
	}
}

func TestHandleGetEntry_NotFound(t *testing.T) {
	store := NewVaultStore()
	handler := HandleGetEntry(store)

	req := httptest.NewRequest(http.MethodGet, "/api/entries/get?id=nonexistent", nil)
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusNotFound {
		t.Fatalf("expected 404, got %d", rr.Code)
	}
}

func TestHandleGetEntry_MissingID(t *testing.T) {
	store := NewVaultStore()
	handler := HandleGetEntry(store)

	req := httptest.NewRequest(http.MethodGet, "/api/entries/get", nil)
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusBadRequest {
		t.Fatalf("expected 400, got %d", rr.Code)
	}
}

func TestHandleDeleteEntry_Success(t *testing.T) {
	store := NewVaultStore()
	added := store.Add(Entry{Name: "ToDelete"})

	handler := HandleDeleteEntry(store)

	req := httptest.NewRequest(http.MethodDelete, "/api/entries/delete?id="+added.ID, nil)
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusOK {
		t.Fatalf("expected 200, got %d", rr.Code)
	}

	if _, ok := store.Get(added.ID); ok {
		t.Error("entry should have been deleted")
	}
}

func TestHandleDeleteEntry_NotFound(t *testing.T) {
	store := NewVaultStore()
	handler := HandleDeleteEntry(store)

	req := httptest.NewRequest(http.MethodDelete, "/api/entries/delete?id=nonexistent", nil)
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusNotFound {
		t.Fatalf("expected 404, got %d", rr.Code)
	}
}

func TestHandleDeleteEntry_MissingID(t *testing.T) {
	store := NewVaultStore()
	handler := HandleDeleteEntry(store)

	req := httptest.NewRequest(http.MethodDelete, "/api/entries/delete", nil)
	rr := httptest.NewRecorder()

	handler.ServeHTTP(rr, req)

	if rr.Code != http.StatusBadRequest {
		t.Fatalf("expected 400, got %d", rr.Code)
	}
}
