package api

type HealthResponse struct {
	Status  string `json:"status"`
	Version string `json:"version"`
}

type ErrorResponse struct {
	Error   string `json:"error"`
	Code    int    `json:"code"`
}

type VaultUnlockRequest struct {
	Password string `json:"password"`
	Path     string `json:"path"`
}

type VaultUnlockResponse struct {
	Token string `json:"token"`
}

type VaultLockResponse struct {
	Status string `json:"status"`
}

type EntryRequest struct {
	Name     string `json:"name"`
	Username string `json:"username"`
	Password string `json:"password"`
	URL      string `json:"url"`
	Notes    string `json:"notes"`
}

type Entry struct {
	ID       string `json:"id"`
	Name     string `json:"name"`
	Username string `json:"username"`
	Password string `json:"password"`
	URL      string `json:"url"`
	Notes    string `json:"notes"`
}

type EntryListResponse struct {
	Entries []Entry `json:"entries"`
	Count   int     `json:"count"`
}
