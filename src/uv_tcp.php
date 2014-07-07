<?hh
class UVTcp
{
    private ?resource $_rs = null;
    <<__Native>> function __construct(UVLoop $loop):void;
    <<__Native>> function __destruct():void;
    <<__Native>> function listen(string $host, int $port, mixed $onConnectCallback):bool;
    <<__Native>> function connect(string $host, int $port, mixed $onConnectCallback):bool;
    <<__Native>> function shutdown(mixed $onShutdownCallback):bool;    
    <<__Native>> function accept(): UVTcp;
    <<__Native>> function close(): void;
    <<__Native>> function setCallback(mixed $onRead, mixed $onWrite, mixed $onError): void;
    <<__Native>> function write(string $buf): bool;
    <<__Native>> function getSockname(): string;
    <<__Native>> function getPeername(): string;
    <<__Native>> function getSockport(): int;
    <<__Native>> function getPeerport(): int;    
}
