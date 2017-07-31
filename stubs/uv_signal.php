<?php

class UVSignal {
    function __construct(UVLoop $loop = NULL) {
    }

    /**
     * @param mixed $cb
     * @param int $signo
     * @return int
     */
    function start($cb, $signo) {
    }

    /**
     * @return int
     */
    function stop() {
    }

    /**
     * @return void
     */
    function __destruct() {
    }
}
