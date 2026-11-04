package main

import "sync"

// sync.Mutex 和 sync.RWMutex 都是不可重入
// - Go 不提供 Goroutine ID 的官方获取接口，无法进程线程的身份识别

type Counter struct {
	mu sync.Mutex
	n  int
}

func (c *Counter) Add(n int) {
	c.mu.Lock()
	defer c.mu.Unlock()

	c.n += n
}

func (c *Counter) AddTwice(n int) {
	c.mu.Lock()
	defer c.mu.Unlock()

	c.Add(n) // 死锁
	c.Add(n)
}
