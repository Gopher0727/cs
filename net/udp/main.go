package main

import (
	"context"
	"flag"
	"fmt"
	"log"
	"net"
	"strconv"
	"syscall"
)

func main() {
	mode := flag.String("mode", "listen", "listen：接收消息；send：发送一条消息")
	name := flag.String("name", "client", "接收端名称，用来区分不同终端")
	port := flag.Int("port", 8091, "所有接收端和发送端使用相同的 UDP 端口")
	addr := flag.String("addr", "127.255.255.255", "目标 IPv4 地址；默认是本机回环广播地址")
	message := flag.String("message", "Hello UDP broadcast!", "要发送的消息")
	flag.Parse()

	if *port < 1 || *port > 65535 {
		log.Fatal("port 必须在 1 到 65535 之间")
	}

	var err error
	switch *mode {
	case "listen":
		err = listen(*name, *port)
	case "send":
		err = send(*addr, *port, *message)
	default:
		log.Fatal("mode 必须是 listen 或 send")
	}
	if err != nil {
		log.Fatal(err)
	}
}

func listen(name string, port int) error {
	// 绑定通配地址，才能接收发往本机各 IPv4 广播地址的数据报。
	conn, err := openUDP(net.JoinHostPort("0.0.0.0", strconv.Itoa(port)), true)
	if err != nil {
		return err
	}
	defer conn.Close()

	fmt.Printf("[%s] listening on %s\n", name, conn.LocalAddr())
	buffer := make([]byte, 65535)
	for {
		n, from, err := conn.ReadFrom(buffer)
		if err != nil {
			return err
		}
		fmt.Printf("[%s] received from %s: %q\n", name, from, string(buffer[:n]))
	}
}

func send(address string, port int, message string) error {
	target, err := net.ResolveUDPAddr("udp4", net.JoinHostPort(address, strconv.Itoa(port)))
	if err != nil {
		return err
	}

	conn, err := openUDP("0.0.0.0:0", false)
	if err != nil {
		return err
	}
	defer conn.Close()

	// 只发送一次；没有遍历客户端，也没有分别向每个客户端发送。
	n, err := conn.WriteTo([]byte(message), target)
	if err != nil {
		return err
	}
	fmt.Printf("sent %d bytes from %s to %s: %q\n", n, conn.LocalAddr(), target, message)
	return nil
}

func openUDP(address string, reuse bool) (net.PacketConn, error) {
	config := net.ListenConfig{
		// Control 在 bind 绑定端口之前执行，多个接收端都必须先开启地址复用。
		Control: func(network, address string, raw syscall.RawConn) error {
			var optionErr error
			if err := raw.Control(func(fd uintptr) {
				if reuse {
					// 允许多个进程绑定到同一个地址和端口
					optionErr = syscall.SetsockoptInt(int(fd), syscall.SOL_SOCKET, syscall.SO_REUSEADDR, 1)
					if optionErr != nil {
						return
					}
				}
				// 允许 socket 发送 IPv4 广播数据报。
				optionErr = syscall.SetsockoptInt(int(fd), syscall.SOL_SOCKET, syscall.SO_BROADCAST, 1)
			}); err != nil {
				return err
			}
			return optionErr
		},
	}
	return config.ListenPacket(context.Background(), "udp4", address)
}
