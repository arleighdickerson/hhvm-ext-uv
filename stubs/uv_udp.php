<?php

class UVUdp {
    /**
     * UVUdp constructor.
     * @param null|UVLoop $loop
     */
    function __construct(UVLoop $loop = null) {
    }

    /**
     * @param string $host
     * @param int $port
     * @return int
     */
    function bind(string $host, int $port) {
    }

    /**
     *
     */
    function close() {
    }

    /**
     * @param mixed $onRecv
     * @param mixed $onSend
     * @param mixed $onError
     * @return int
     */
    function setCallback(mixed $onRecv, mixed $onSend, mixed $onError) {
    }

    /**
     * @param string $dest
     * @param int $port
     * @param string $buf
     * @return int
     */
    function sendTo(string $dest, int $port, string $buf) {
    }

    /**
     * @return string
     */
    function getSockname() {
    }

    /**
     * @return int
     */
    function getSockport() {
    }
}
