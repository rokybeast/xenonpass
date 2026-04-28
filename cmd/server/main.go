package main

import (
	"fmt"
	"xenonpass/api"
)

func main() {
	srv := api.NewServer(":8080")
	fmt.Println("xenonpass API server listening on :8080")
	if err := srv.ListenAndServe(); err != nil {
		fmt.Println("server error:", err)
	}
}
