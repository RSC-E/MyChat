# MyChat

## <span id="目录">目录</span>
[1.了解My Chat](#了解) \
[2.更新历史](#更新) \
[3.研发团队](#团队)

### <span id="了解">了解MyChat软件</span>
> 这是一个开放性的Windows管道通信程序，原理和WeChat、DingTalk、TikTok相似，可以建群聊 \
> 这个程序认准界面(Github Pages)[https://www.github.com],其余全是盗版 \
> 本软件仅供学习和研究使用，切勿用于其他用途（如：商业通途）

```mermaid
graph TD
    A[客户端A] -->|TCP连接| B[聊天服务器]
    C[客户端B] -->|TCP连接| B
    D[客户端C] -->|TCP连接| B
    B -->|广播消息| A
    B -->|广播消息| C
    B -->|广播消息| D

```
```mermaid
graph LR
    LB[负载均衡] --> S1[服务器节点1]
    LB --> S2[服务器节点2]
    LB --> S3[服务器节点3]
    S1 --> DB[(数据库集群)]
    S2 --> DB
    S3 --> DB
    DB -->|主从同步| Backup[备份节点]
```
```mermaid
graph TB
    subgraph 高性能设计
        A[I/O多路复用] --> B[线程池]
        B --> C[无锁队列]
        C --> D[零拷贝传输]
        D --> E[批处理]
    end
```
```mermaid
gantt
    title 心跳检测周期
    dateFormat  HH:mm:ss
    section 客户端
    发送心跳请求    :a1, 2023-01-01 00:00:00, 1s
    等待响应       :after a1, 3s
    section 服务端
    检测心跳       :2023-01-01 00:00:02, 2s
    超时处理       :crit, 2023-01-01 00:00:05, 1s]\
```
```mermaid
journey
    title 连接断开处理流程
    section 检测阶段
        检测Socket错误: 5
        确认连接状态: 3
    section 恢复阶段
        清理会话数据: 4
        释放资源: 3
        日志记录: 2
```
```mermaid
sequenceDiagram
    participant A as 客户端A
    participant S as 服务器
    participant B as 客户端B
    
    A->>S: 发送消息"Hello"
    S->>S: 验证消息有效性
    S->>B: 广播消息"Hello"
    B->>B: 显示消息
    S->>A: 返回ACK(可选)
```
```mermaid
sequenceDiagram
    participant ClientA
    participant Server
    participant ClientB
    
    ClientA->>Server: 发送消息"Hello"
    Server->>Server: 接收并验证消息
    Server->>ClientB: 广播消息"Hello"
    ClientB->>ClientB: 显示消息
    Server->>ClientA: 返回ACK(可选)
    
    loop 心跳检测
        Server->>ClientA: HEARTBEAT
        ClientA->>Server: HEARTBEAT_ACK
        Server->>ClientB: HEARTBEAT
        ClientB->>Server: HEARTBEAT_ACK
    end
```

### <span id="更新">更新历史</span>
### Release1.0.0
> 第一个版本

### <span id="团队">研发团队</span>
### RSC Studio
> SYSTEM-WIN12-ZDY [Giithub Page](https://www.github.com/SYSTEM-WIN12-ZDY)
