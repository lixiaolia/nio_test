package com.sample.server;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.CancelledKeyException;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.util.Iterator;
import java.util.Scanner;

public class NIOServer implements Runnable {

    private Selector selector;
    private ByteBuffer readBuffer = ByteBuffer.allocate(1024);
    private ByteBuffer writeBuffer = ByteBuffer.allocate(1024);

    public NIOServer(int port) { init(port); }

    public void initServer(int port) throws IOException { init(port); }

    private void init(int port) {
        try {
            System.out.println("Server starting at port: " + port);
            // 开启多路复用器
            this.selector = Selector.open();
            // 开启服务通道
            ServerSocketChannel serverChannel = ServerSocketChannel.open();
            serverChannel.configureBlocking(false);
            serverChannel.bind(new InetSocketAddress(port));
            serverChannel.register(this.selector, SelectionKey.OP_ACCEPT);
            System.out.println("Server started");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void run() {
        while (true) {
            try {
                this.selector.select();
                Iterator<SelectionKey> keys = this.selector.selectedKeys().iterator();
                while (keys.hasNext()) {
                    SelectionKey key = keys.next();
                    keys.remove();
                    if (key.isValid()) {
                        try {
                            if (key.isAcceptable()) {
                                accept(key);
                            }
                        } catch (CancelledKeyException cke) {
                            key.cancel();
                        }

                        try {
                            if (key.isReadable()) {
                                read(key);
                            }
                        } catch (CancelledKeyException cke) {
                            key.cancel();
                        }

                        try {
                            if (key.isWritable()) {
                                write(key);
                            }
                        } catch (CancelledKeyException cke) {
                            key.cancel();
                        }
                    }
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private void accept(SelectionKey key) {
        try {
            ServerSocketChannel serverChannel = (ServerSocketChannel) key.channel();
            SocketChannel channel = serverChannel.accept();
            channel.configureBlocking(false);
            channel.register(this.selector, SelectionKey.OP_READ);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void read(SelectionKey key) {
        try {
            this.readBuffer.clear();
            SocketChannel channel = (SocketChannel) key.channel();
            int readLength = channel.read(readBuffer);
            if (readLength == -1) {
                key.channel().close();
                key.cancel();
                return;
            }
            this.readBuffer.flip();
            byte[] datas = new byte[readBuffer.remaining()];
            readBuffer.get(datas);
            System.out.println("From " + channel.getRemoteAddress() + "Client: " + new String(datas, "UTF-8"));
            channel.register(this.selector, SelectionKey.OP_WRITE);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void write(SelectionKey key) {
        this.writeBuffer.clear();
        SocketChannel channel = (SocketChannel) key.channel();
        Scanner reader = new Scanner(System.in);
        try {
            String line = reader.nextLine();
            writeBuffer.put(line.getBytes("UTF-8"));
            writeBuffer.flip();
            channel.write(writeBuffer);
            channel.register(this.selector, SelectionKey.OP_READ);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static void main(String[] args) throws IOException {
        new Thread(new NIOServer(6666)).start();
    }
}
