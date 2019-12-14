# captor

MySql UDF module for data capture. It transmits data from the trigger to capsrvd.

## nectat

netcat(address, method, exchange, json-sting, route);

* address - may be
```
    localhost:port
    port
```
> working wihout answer from capsrvd

```
    -port 
    -localhost:port
```
> is ignoring connection error when capsrvd is epsent

```
    +port
    +localhost:port
```
> working with wait of answer from capsrvd

>captor is designed for using via local connections only
>	using captor over internet may slowdown you mysql

* method - usualy
```
    create - sql insert
    modify - sql update
    remove - sql delete
```
> and must be simple string
> without any escape symbols

* exchange - rabbitmq exchange
> may be null

* json-data - string witout escape chars

> use capjs for this

* route
```
    number or string
```
> may be null

### example sql
```
select netcat(6033, "select", "user", 
    jsobj(42 as 'id', 'peter' as 'name', 'male' as 'sex'), "user-id=42");
```
### output
```
# nc -l -p 6033 127.0.0.1
```
```
[1576330299387,"select","user",[{"id":42,"name":"peter","sex":"male"},"user-id=42"]]
```

## installig
```
CREATE FUNCTION netcat RETURNS INTEGER SONAME 'libcaptor.so';
```
