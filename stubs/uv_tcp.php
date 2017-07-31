<?php

class UVTcp {
    /**
     * UVTcp constructor.
     * @param null|UVLoop $loop
     */
    function __construct(UVLoop $loop = null) {
    }

    /**
     * @param string $host
     * @param int $port
     * @param mixed $onConnectCallback
     * @return int
     */
    function listen(string $host, int $port, mixed $onConnectCallback) {
    }

    /**
     * @param string $host
     * @param int $port
     * @param mixed $onConnectCallback
     * @return int
     */
    function connect(string $host, int $port, mixed $onConnectCallback) {
    }

    /**
     * @param mixed $onShutdownCallback
     * @return int
     */
    function shutdown(mixed $onShutdownCallback) {
    }

    /**
     * @return UVTcp
     */
    function accept() {
    }

    /**
     *
     */
    function close() {
    }

    /**
     * @param mixed $onRead
     * @param mixed $onWrite
     * @param mixed $onError
     */
    function setCallback(mixed $onRead, mixed $onWrite, mixed $onError) {
    }

    /**
     * @param string $buf
     * @return int
     */
    function write(string $buf) {
    }

    /**
     * @return string
     */
    function getSockname() {
    }

    /**
     * @return string
     */
    function getPeername() {
    }

    /**
     * @return int
     */
    function getSockport() {
    }

    /**
     * @return int
     */
    function getPeerport() {
    }
}
