# captor

MySql UDF module for data capture. It transmits data from the trigger to capsrvd.

## nectat

netcat(address, method, exchange, json-sting, route);

* address - may be
```
    localhost:port
    port
```
> for ignoring connection error when capsrvd is epsent
```
    -port 
    -localhost:port
```
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

### trigger
> it use capjs udf module
```
CREATE TRIGGER `user_AFTER_INSERT` AFTER INSERT ON `user` FOR EACH ROW
BEGIN
   SET @message_len = (
    SELECT 
        netcat(6093, 'create', 'user',
        jsobj(
        NEW.`id` as 'id',
        NEW.`login` as 'login',
        NEW.`name` as `name,`
        NEW.`sex` as `sex`,
        unix_timestamp(CONVERT_TZ(NEW.`create_at`, @@global.time_zone, @@session.time_zone)) * 1000 as 'create_at',
        NEW.`comment` as 'comment',
        cast(round(unix_timestamp(now(4))*1000) as unsigned) as 'event_at'
        ), NEW.`id`));
END
```
```
CREATE TRIGGER `user_AFTER_UPDATE` AFTER UPDATE ON `user` FOR EACH ROW
BEGIN
    SET @message_len = (
    SELECT
        netcat(6093, 'modify', 'user',
        jsobj(
        NEW.`id` as 'id',
        NEW.`login` as 'login',
        NEW.`name` as `name`,
        NEW.`sex` as `sex`,
        unix_timestamp(CONVERT_TZ(NEW.`create_at`, @@global.time_zone, @@session.time_zone)) * 1000 as 'create_at',
        NEW.`comment` as 'comment',
        cast(round(unix_timestamp(now(4))*1000) as unsigned) as 'event_at'
        ), NEW.`id`));
END
```
```
CREATE TRIGGER `user_AFTER_DELETE` AFTER DELETE ON `user` FOR EACH ROW
BEGIN
    SET @message_len = (
    SELECT
        netcat(6093, 'remove', 'user',
        jsobj(
            OLD.`id` as 'id',
            cast(round(unix_timestamp(now(4))*1000) as unsigned) as 'event_at'
        ), OLD.`id`));
END
```