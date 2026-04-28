package api

import "net/http"

type Server struct {
	Addr   string
	mux    *http.ServeMux
	store  *VaultStore
	cors   CORSConfig
}

func NewServer(addr string) *Server {
	s := &Server{
		Addr:  addr,
		mux:   http.NewServeMux(),
		store: NewVaultStore(),
		cors:  DefaultCORSConfig(),
	}
	s.routes()
	return s
}

func NewServerWithCORS(addr string, cors CORSConfig) *Server {
	s := &Server{
		Addr:  addr,
		mux:   http.NewServeMux(),
		store: NewVaultStore(),
		cors:  cors,
	}
	s.routes()
	return s
}

func (s *Server) routes() {
	s.mux.HandleFunc("/api/health", HandleHealth())
	s.mux.HandleFunc("/api/entries", HandleListEntries())
	s.mux.HandleFunc("/api/entries/create", HandleCreateEntry())
	s.mux.HandleFunc("/api/entries/delete", HandleDeleteEntry())
}

func (s *Server) Handler() http.Handler {
	return CORSMiddleware(s.cors)(s.mux)
}

func (s *Server) ListenAndServe() error {
	return http.ListenAndServe(s.Addr, s.Handler())
}
