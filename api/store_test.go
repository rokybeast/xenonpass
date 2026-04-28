package api

import (
	"sync"
	"testing"
)

func TestVaultStore_Add(t *testing.T) {
	store := NewVaultStore()
	entry := store.Add(Entry{Name: "Test", Username: "user1"})

	if entry.ID != "entry-1" {
		t.Errorf("expected ID entry-1, got %q", entry.ID)
	}
	if entry.Name != "Test" {
		t.Errorf("expected name Test, got %q", entry.Name)
	}
}

func TestVaultStore_AddIncrementsID(t *testing.T) {
	store := NewVaultStore()

	e1 := store.Add(Entry{Name: "First"})
	e2 := store.Add(Entry{Name: "Second"})
	e3 := store.Add(Entry{Name: "Third"})

	if e1.ID != "entry-1" {
		t.Errorf("expected entry-1, got %q", e1.ID)
	}
	if e2.ID != "entry-2" {
		t.Errorf("expected entry-2, got %q", e2.ID)
	}
	if e3.ID != "entry-3" {
		t.Errorf("expected entry-3, got %q", e3.ID)
	}
}

func TestVaultStore_Get(t *testing.T) {
	store := NewVaultStore()
	added := store.Add(Entry{Name: "Findme", Password: "secret"})

	got, ok := store.Get(added.ID)
	if !ok {
		t.Fatal("expected to find entry")
	}
	if got.Name != "Findme" {
		t.Errorf("expected name Findme, got %q", got.Name)
	}
	if got.Password != "secret" {
		t.Errorf("expected password secret, got %q", got.Password)
	}
}

func TestVaultStore_GetNotFound(t *testing.T) {
	store := NewVaultStore()

	_, ok := store.Get("entry-999")
	if ok {
		t.Error("expected not found")
	}
}

func TestVaultStore_List(t *testing.T) {
	store := NewVaultStore()
	store.Add(Entry{Name: "A"})
	store.Add(Entry{Name: "B"})

	entries := store.List()
	if len(entries) != 2 {
		t.Errorf("expected 2 entries, got %d", len(entries))
	}
}

func TestVaultStore_ListEmpty(t *testing.T) {
	store := NewVaultStore()
	entries := store.List()

	if entries == nil {
		t.Error("expected non-nil empty slice")
	}
	if len(entries) != 0 {
		t.Errorf("expected 0 entries, got %d", len(entries))
	}
}

func TestVaultStore_Delete(t *testing.T) {
	store := NewVaultStore()
	added := store.Add(Entry{Name: "Deleteme"})

	ok := store.Delete(added.ID)
	if !ok {
		t.Error("expected delete to succeed")
	}

	_, found := store.Get(added.ID)
	if found {
		t.Error("expected entry to be gone after delete")
	}
}

func TestVaultStore_DeleteNotFound(t *testing.T) {
	store := NewVaultStore()

	ok := store.Delete("entry-999")
	if ok {
		t.Error("expected delete of nonexistent to return false")
	}
}

func TestVaultStore_ConcurrentAccess(t *testing.T) {
	store := NewVaultStore()
	var wg sync.WaitGroup

	for i := 0; i < 100; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			store.Add(Entry{Name: "concurrent"})
		}()
	}

	wg.Wait()

	entries := store.List()
	if len(entries) != 100 {
		t.Errorf("expected 100 entries, got %d", len(entries))
	}
}

func TestFormatID(t *testing.T) {
	tests := []struct {
		input    int
		expected string
	}{
		{1, "entry-1"},
		{10, "entry-10"},
		{100, "entry-100"},
	}

	for _, tc := range tests {
		got := formatID(tc.input)
		if got != tc.expected {
			t.Errorf("formatID(%d) = %q, want %q", tc.input, got, tc.expected)
		}
	}
}

func TestItoa(t *testing.T) {
	tests := []struct {
		input    int
		expected string
	}{
		{0, "0"},
		{1, "1"},
		{42, "42"},
		{999, "999"},
	}

	for _, tc := range tests {
		got := itoa(tc.input)
		if got != tc.expected {
			t.Errorf("itoa(%d) = %q, want %q", tc.input, got, tc.expected)
		}
	}
}
