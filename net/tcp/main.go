package main

import (
	"log"
	"net"
)

func main() {
	listener, err := net.Listen("tcp6", "[::1]:8090")
	if err != nil {
		log.Fatal(err)
	}
	defer listener.Close()

	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Print(err)
			continue
		}

		if _, err := conn.Write([]byte("Hello world from Go!\n")); err != nil {
			log.Print(err)
		}
		conn.Close()
	}
}
