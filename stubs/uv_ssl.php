<?php

class UVSSL extends UVTcp {
    /**
     * UVSSL constructor.
     * @param null|UVLoop $loop
     * @param int $sslMethod
     * @param int $nContexts
     */
    function __construct(UVLoop $loop = null, int $sslMethod = self::SSL_METHOD_TLSV1, int $nContexts = 1) {
    }

    /**
     * @return UVSSL
     */
    function accept() {
    }

    /**
     * @param mixed $callback
     */
    function setSSLHandshakeCallback(mixed $callback) {
    }

    /**
     * @param mixed $callback
     */
    function setSSLServerNameCallback(mixed $callback) {
    }

    /**
     * @param string $buf
     * @return int
     */
    function write(string $buf) {
    }

    /**
     * @param string $cert
     * @param int $n
     * @return bool
     */
    function setCert(string $cert, int $n = 0) {
    }

    /**
     * @param string $privateKey
     * @param int $n
     * @return bool
     */
    function setPrivateKey(string $privateKey, int $n = 0) {
    }

    /**
     * @param string $host
     * @param int $port
     * @param mixed $onConnectCallback
     * @return int
     */
    function connect(string $host, int $port, mixed $onConnectCallback) {
    }
}
