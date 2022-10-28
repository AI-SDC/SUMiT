package uwecellsuppressionserver;

import java.io.IOException;
import java.net.Socket;

/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author cns
 */
public class RequestHandler implements Runnable {

    Socket socket;

    public RequestHandler(Socket socket) {
        this.socket = socket;
    }

    @Override
    public void run() {
        UWECellSuppressionServer.HandleRequest(socket);

        try {
            socket.close();
        } catch (IOException e) {
            System.out.println(e);
            System.exit(1);
        }

        // Encourage release of the underlying system resources
        socket = null;
//        System.runFinalization();
//        System.gc();
    }

}
