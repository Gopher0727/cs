```bash
sudo pacman -S --needed gcc ccache hiredis pkgconf

pkg-config --modversion hiredis # 确认安装成功
```

- 给缓存加过期时间的时间附带随机值，防止雪崩
- allkeys-lru 确实省心，但是如果 redis 里面存在一些不能丢掉的数据，那必须使用 volatile-lru

> Q：缓存和数据库怎么保持一致？
>
> A: 最常见的 Cache Aside。读时，查缓存，有就返回，没有就查数据库，然后**写回**缓存；写时，先更新数据库，再**删**缓存，一定要删，而不是改，因为并发改可能写入旧值。更严谨的做法，把“删缓存”放到消息队列重试，或者用 binlog 异步补偿，保障最终一致性。追求强一致，需要分布式事务，成本会很高。

> 分布式锁：NX EX
>
> 1. 先 SET 再 EXPIRE，如果两条命令中间宕机就死锁。
>
> 2. 直接 DEL 解锁，可能会删掉别人的锁。value 要存自己的身份标识。
>
> 3. 业务比过期时间长，导致锁提前释放，因此需要自动续期。
