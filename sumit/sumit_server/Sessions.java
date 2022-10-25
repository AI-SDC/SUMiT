/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package uwecellsuppressionserver;

import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import java.util.NoSuchElementException;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 *
 * @author cns
 */
public class Sessions {

    public final int limit;

    private final ConcurrentHashMap<String, Session> sessions = new ConcurrentHashMap();
    private final Object sessionsLock = new Object();

    public Sessions(int limit) {
        this.limit = limit;
    }

    // Clean up open sessions but do not remove from map to avoid concurrency issues (with no synchronisation) or potential deadlock issues (with synchronisation)
    public void shutdown() {
        Set<Map.Entry<String, Session>> entrySet = sessions.entrySet();
        Iterator<Map.Entry<String, Session>> it = entrySet.iterator();
        while (it.hasNext()) {
            try {
                it.next().getValue().close();
            } catch (NoSuchElementException e) {
                // The only session has just been removed concurrently by another thread - ignore
            }
        }
    }

    public void checkForOrphans() {
        synchronized(sessionsLock) {
            Set<Map.Entry<String, Session>> entrySet = sessions.entrySet();
            Iterator<Map.Entry<String, Session>> it = entrySet.iterator();
            while (it.hasNext()) {
                try {
                    Session session = it.next().getValue();
                    if (session.orphanned()) {
                        Logger.log("Session orphaned: %s", session.id);
                        sessions.remove(session.id);
                        session.close();
                    }
                } catch (NoSuchElementException e) {
                    // The only session has just been removed concurrently by another thread - ignore
                }
            }
        }
    }

    public Session create() {
        synchronized(sessionsLock) {
            if (sessions.size() < limit) {
                Session session = new Session();
                sessions.put(session.id, session);

                return session;
            } else {
                return null;
            }
        }
    }

    public void destroy(Session session) {
        synchronized(sessionsLock) {
            sessions.remove(session.id);
            session.close();
        }
    }

    public Session get(String key) {
        return sessions.get(key);
    }

    public Iterator<Entry<String, Session>> iterator() {
        return sessions.entrySet().iterator();
    }

}
