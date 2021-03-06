<?php
require __DIR__ . '/../../test-tools.php';
$host = '127.0.0.1';
$port = rand(5000, 9000);
$pid = pcntl_fork();
if($pid){
    usleep(100000);    

    $fp = stream_socket_client("tcp://$host:$port", $errno, $errstr);
    $randomData = sha1(rand());
    fwrite($fp, $randomData);
    Equal($randomData, fread($fp, 1000), "Echo Server");
    fclose($fp);
    
    $fp = stream_socket_client("tcp://$host:$port", $errno, $errstr);
    fclose($fp);
    
    usleep(100000); //wait server
    
    $fp = stream_socket_client("tcp://$host:$port", $errno, $errstr);
    Equal('client closed', fread($fp, 1000), "Client close Trigger");
    fclose($fp);
    
    $fp = stream_socket_client("tcp://$host:$port", $errno, $errstr);
    fwrite($fp, '!close!');
    fclose($fp);
    
    pcntl_waitpid($pid, $status);
    exit;
}
$loop = UVLoop::defaultLoop();
$server = new UVTcp($loop);
$server->clientCloseTriggered = false;
Equal(0, $server->listen($host, $port, function($server) {
    $client = $server->accept();
    $client->setCallback(function($client, $recv) use($server){
        $client->write($recv);    
        if($recv == '!close!'){            
            $server->close();
        }
    }, function($client, $status){
        $client->close();
    }, function($client) use($server){
        $server->clientCloseTriggered = true;
        $client->close();
    });
    
    if($server->clientCloseTriggered){
        $client->write("client closed");
        $server->clientCloseTriggered = false;
    }    
}), "Server listen");
$loop->run();