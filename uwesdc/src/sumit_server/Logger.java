/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package uwecellsuppressionserver;

import java.io.FileWriter;
import java.io.IOException;

/**
 *
 * @author cns
 */
public class Logger {
    
    private static final Logger instance;
    
    private static FileWriter writer = null;
    
    static {
        instance = new Logger();
    }
    
    private Logger() {
        try {
            writer = new FileWriter("ServerLog.txt");
        } catch (IOException e) {
            writer = null;
        }
    }
    
    public final static Logger getInstance() {
        return instance;
    }

    public synchronized void close() {
        if (writer != null) {
            try {
                writer.close();
                writer = null;
            } catch (IOException e) {
                // Ignore
            }
        }
    }
    
    public static void log(String format, Object ... args) {
        if (writer != null) {
            long now = System.currentTimeMillis();
            StringBuilder sb = new StringBuilder();
            sb.append(String.format("%d.%d %d ", now / 1000, now % 1000, Thread.currentThread().getId()));
            sb.append(String.format(format, args));
            sb.append(System.lineSeparator());
            try {
                // Writer.write handles synchronization
                writer.write(sb.toString());
                writer.flush();
            } catch (IOException e) {
                // Ignore
            }
        }
    }
    
}
