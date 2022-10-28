package uwecellsuppressionserver;

/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author cns
 */
public class OrphanHandler implements Runnable {

    private final Sessions sessions;

    public OrphanHandler(Sessions sessions) {
        this.sessions = sessions;
    }

    @Override
    public void run() {
            try {
                while (true) {
                    sessions.checkForOrphans();

                    Thread.sleep(10000);
                }
            } catch (InterruptedException e) {
                // Exit thread
            }
    }

}
