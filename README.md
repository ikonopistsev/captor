# captor

MySql UDF module for data capture. It transmits data from the trigger to capsrvd.

## nectat

Рассылка данных

* 1й параметр - порт, тогда рассылка идет localhost:port либо адрес с портом "host:port"
> отрицательный порт отключает выдачу ошибки при неудачной рассылке

* 2й параметр - название команды. 
> В триггерах предполагается использовать:
>    create, update, remove
>    для тестов select

* 3й параметр - используется в качестве exchange и ключа маршрута
> предполагается использование имени таблицы

* 4й параметр - json данные

* 5й параметр - ключевые маршруты (если указаны)

### Маршруты

Используется для формирования rounting key
Можно сформировать несколько видов маршрутов

* 1. простой - ".1232456" - чистый id
```
, `login` as '' - формируется при отсутсвии названия параметра
```
* 2. обычный - ".login=1232456" 
```
, `login` 
```
* 3. сложный - .login=1232456.source=3 - формируется вручную или через json
```
, jsobj(`login`, `source`)
```

### Пример запроса
```
SELECT 
    netcat(6033, 'select', 'order', 
        jsobj(  `ticket`, `login`, `source`, 
                `cmd`, `symbol`, `volume`, 
                `open_time`, `open_price`, 
                `close_time`, `close_price`, 
                `swap`, `profit`, `stop_loss`, 
                `take_profit`, `comment`, 
                UNIX_TIMESTAMP(`update_time`) as 'update_time.u'
        )
        , jsobj(`login`, `source`)  # сложный маршрут
        , `login`                   # простой маршрут с идентификатором
        , `id` as ''                # простой маршрут без идентификатора
        , mkkv(`symbol`)            # строковый маршрут
    ) as 'result'
FROM 
    `order`
LIMIT 3
;
```
### Вывод
```
# nc -l -p 6033 127.0.0.1
```
```
1536677138466 select order

{"ticket":45860326,"login":2781696,"source":1,"cmd":1,"symbol":"EURUSD","volume":5000,"open_time":1510073782000,"open_price":1.15741,"close_time":1510682711000,"close_price":1.17902,"swap":-1.2,"profit":-717.29,"stop_loss":0,"take_profit":0,"comment":"[stopout]","update_time":"2018-09-05T17:56:22.000Z"}
{"login":2781696,"source":1}
login=2781696
97609908220178877
symbol=EURUSD

{"ticket":45860933,"login":2890240,"source":1,"cmd":0,"symbol":"EURUSD","volume":5000,"open_time":1510075241000,"open_price":1.15884,"close_time":1510075247000,"close_price":1.15869,"swap":0,"profit":-0.75,"stop_loss":0,"take_profit":0,"comment":"","update_time":"2018-09-05T16:07:24.000Z"}
{"login":2890240,"source":1}
login=2890240
97609908220395293
symbol=EURUSD

{"ticket":45865407,"login":2879488,"source":1,"cmd":6,"symbol":"aaBB","volume":1000,"open_time":1510102907000,"open_price":0,"close_time":1510102907000,"close_price":0,"swap":0,"profit":1.08,"stop_loss":0,"take_profit":0,"comment":"IB rebate (hhf) #1775","update_time":"2018-09-05T17:56:22.000Z"}
{"login":2879488,"source":1}
login=2879488
97609908220322844
symbol=aaBBSD
```

## Установка
```
CREATE FUNCTION netcat RETURNS INTEGER SONAME 'libcaptor.so';
```
