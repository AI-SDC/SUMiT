package uwecellsuppressionserver;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.URI;
import java.net.URISyntaxException;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.InvalidPathException;
import java.nio.file.Path;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.NoSuchElementException;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

public class UWECellSuppressionServer {

    private static final String version = "1.7.0";
    private static final int protocol = 4;
    private static final int additionalCores = -1;

    public static File store;

    private static final int cores = Runtime.getRuntime().availableProcessors();
    // Extra thread below is to check for orphaned sessions and is extremely lightweight
    private static final Executor executor = Executors.newFixedThreadPool(cores + additionalCores + 1);
//    private static final TimeZone tz = TimeZone.getTimeZone("UTC");

    private static final Sessions sessions = new Sessions(cores + additionalCores);

    public static void main(String[] args) {
        Logger.log("UWECellSuppressionServer v" + version);
        Logger.log(System.getProperty("os.name") + " " + System.getProperty("os.version") + " " + System.getProperty("os.arch"));
        Logger.log("Java " + System.getProperty("java.version") + " "+ System.getProperty("java.vendor"));
        Logger.log(Runtime.getRuntime().availableProcessors() + " cores");
        Logger.log(additionalCores + " additional cores");
        
        try {
            // Create directory to store files
            Path path = FileSystems.getDefault().getPath("store");
            store = path.toFile();
            try {
                Files.createDirectory(path);
            } catch (FileAlreadyExistsException e) {
                // Ignore
            } catch (IOException e) {
                Logger.log(e.getMessage());
                System.exit(1);
            }

            // Delete all files in store
            File[] files = store.listFiles();
            if (files != null) {
                for (File file : files) {
                    if (file.isFile()) {
                        file.delete();
                    }
                }
            }

            // Create shutdown hook
            Runtime.getRuntime().addShutdownHook(new Thread() {
                @Override
                public void run() {
                    sessions.shutdown();
                    Logger.getInstance().close();
                }
            });

            // Check for orphaned sessions in another thread
            executor.execute(new OrphanHandler(sessions));

            // Handle connections
            try {
                ServerSocket serverSocket = new ServerSocket(1081);
                
                while (true) {
                    Socket socket = serverSocket.accept();
                    executor.execute(new RequestHandler(socket));
                    socket = null;

                    // Give the garbage collector a chance to run
                    Thread.sleep(10);
                }
            } catch (IOException e) {
                Logger.log(e.getMessage());
                System.exit(1);
            }
        } catch (InterruptedException e) {
            Logger.log("Aborting");
            
            // Set the error status to the same as used by Bash when script terminated by control-c
            System.exit(130);
        }
    }
    
    private static Session getSession(String query) {
        if (query == null) {
            return null;
        }
        
        String[] elements = query.split("&");
        for (String s : elements) {
            if (s.startsWith("session=")) {
                String[] keyValue = s.split("=");
                if (keyValue.length == 2) {
                    Session session = sessions.get(keyValue[1]);
                    if (session != null) {
                        return session;
                    } else {
                        Logger.log("Unknown session identifier: %s", keyValue[1]);
                        return null;
                    }
                } else {
                    Logger.log("Badly formatted session identifier: %s", s);
                    return null;
                }
            }
        }
        
        return null;
    }

    public static void HandleRequest(Socket socket) {
        try {
            Logger.log("Connection: %s", socket.getRemoteSocketAddress().toString());

            try (BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()))) {

                String request = in.readLine();

                if (request != null) {
                    Logger.log("Request: %s", request);
                    
                    String[] tokens = request.split(" ");
                    String method = tokens[0];

                    StringBuilder sb = new StringBuilder();
                    int status = 200;
                    String response = "";
                    boolean html = false;

                    try {
                        URI uri = new URI(tokens[1]);
                        String resource = uri.getPath();
                        String query = uri.getQuery();
    //                    Logger.log("Method: %socket", method);
    //                    Logger.log("Resource: %socket", resource);
    //                    if (query != null) {
    //                        Logger.log("Query: %socket", query);
    //                    }
                        Session session;

                        switch(method) {
                            case "GET":
                                switch(resource) {
                                    case "/":
                                        html = true;
                                        sb.append("<html>");
                                        sb.append("<head>");
                                        sb.append("<title>");
                                        sb.append("UWE Cell Suppression Server");
                                        sb.append("</title>");
                                        sb.append("</head>");
                                        sb.append("<body style=\"font-family: Monospace, Courier; font-size:10pt\">");
                                        sb.append("<h1 style=\"font-family: Helvetica, Sans-Serif; font-size:14pt\">");
                                        sb.append("UWE Cell Suppression Server");
                                        sb.append("</h1>");
                                        sb.append("<p>");
                                        sb.append(socket.getLocalAddress().getHostName());
                                        sb.append(" (");
                                        sb.append(socket.getLocalAddress().getHostAddress());
                                        sb.append(")<br>");
                                        sb.append(System.getProperty("os.name"));
                                        sb.append(" ");
                                        sb.append(System.getProperty("os.version"));
                                        sb.append(" ");
                                        sb.append(System.getProperty("os.arch"));
                                        sb.append("<br>");
                                        sb.append("Java ");
                                        sb.append(System.getProperty("java.version"));
                                        sb.append(" ");
                                        sb.append(System.getProperty("java.vendor"));
                                        sb.append("<br>");
                                        sb.append("Server version ");
                                        sb.append(version);
                                        sb.append("<br>");
                                        sb.append(cores);
                                        sb.append(" cores (limit ");
                                        sb.append(sessions.limit);
                                        sb.append(")");
                                        sb.append("</p>");
                                        DateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
//                                        dateFormat.setTimeZone(tz);
                                        Iterator<Entry<String, Session>> it = sessions.iterator();
                                        while (it.hasNext()) {
                                            try {
                                                session = it.next().getValue();
                                                sb.append(dateFormat.format(session.timestamp));
                                                sb.append(" ");
                                                if (session.query == null) {
                                                    sb.append("session=");
                                                    sb.append(session.id);
                                                } else {
                                                    sb.append(session.query);
                                                }
                                                sb.append(" (");
                                                sb.append(session.statusString());
                                                sb.append(")");
                                                sb.append("<br>");
                                            } catch (NoSuchElementException e) {
                                                // The only session has just been removed concurrently by another thread - ignore
                                            }
                                        }
                                        sb.append("</font>");
                                        sb.append("</body>");
                                        sb.append("</html>");
                                        break;

                                    case "/cores":
                                        sb.append(cores);
                                        break;

                                    case "/limit":
                                        sb.append(sessions.limit);
                                        break;

                                    case "/protocol":
                                        sb.append(protocol);
                                        break;

                                    case "/session":
                                        session = sessions.create();
                                        if (session != null) {
                                            sb.append(session.id);
                                        } else {
                                            sb.append("0");
                                        }
                                        break;

                                    case "/close":
                                        session = getSession(query);
                                        if (session != null) {
                                            sessions.destroy(session);
                                        } else {
                                            status = 404;
                                        }
                                        break;

                                    case "/status":
                                        session = getSession(query);
                                        if (session != null) {
                                            session.keepAlive();
                                            sb.append(session.status());
                                        } else {
                                            status = 404;
                                        }
                                        break;

                                    case "/result":
                                        session = getSession(query);
                                        if (session != null) {
                                            sb.append(session.result());
                                        } else {
                                            status = 404;
                                        }
                                        break;

                                    case "/time":
                                        session = getSession(query);
                                        if (session != null) {
                                            sb.append(session.elapsedTime());
                                        } else {
                                            status = 404;
                                        }
                                        break;

                                    case "/file":
                                        session = getSession(query);
                                        if (session != null) {
                                            try {
                                                File outfile = new File(store, session.outfile);
                                                if (Files.exists(outfile.toPath())) {
                                                    try (BufferedReader file = new BufferedReader(new FileReader(outfile))) {
                                                        int c;
                                                        do {
                                                            c = file.read();
                                                            if (c != -1) {
                                                                sb.append((char)c);
                                                            }
                                                        } while (c != -1);
                                                    }
                                                } else {
                                                    Logger.log("Internal error retrieving resource: %s", session.outfile);
                                                    status = 404;
                                                }
                                            } catch (InvalidPathException e) {
                                                Logger.log("Internal error retrieving resource: %s", session.outfile);
                                                status = 404;
                                            }
                                        } else {
                                            status = 404;
                                        }
                                        break;

                                    case "/costs":
                                        session = getSession(query);
                                        if (session != null) {
                                            try {
                                                File costfile = new File(store, session.costfile);
                                                if (Files.exists(costfile.toPath())) {
                                                    try (BufferedReader file = new BufferedReader(new FileReader(costfile))) {
                                                        int c;
                                                        do {
                                                            c = file.read();
                                                            if (c != -1) {
                                                                sb.append((char)c);
                                                            }
                                                        } while (c != -1);
                                                    }
                                                } else {
                                                    Logger.log("Internal error retrieving resource: %s", session.costfile);
                                                    status = 404;
                                                }
                                            } catch (InvalidPathException e) {
                                                Logger.log("Internal error retrieving resource: %s", session.costfile);
                                                status = 404;
                                            }
                                        } else {
                                            status = 404;
                                        }
                                        break;

                                    default:
                                        Logger.log("Unknown resource: %s", resource);
                                        status = 404;
                                }

                                response = sb.toString();
                                break;

                            case "PUT":
                                switch(resource) {
                                    case "/file":
                                        session = getSession(query);
                                        if (session != null) {
                                            String line;
                                            int contentLength = 0;
                                            boolean done = false;
                                            do {
                                                line = in.readLine();
                                                if (line.equals("")) {
                                                    try (BufferedWriter out = new BufferedWriter(new FileWriter(new File(store, session.infile)))) {
                                                        for (int i = 0; i < contentLength; i++) {
                                                            char c = (char)in.read();
                                                            out.append(c);
                                                        }
                                                        done = true;
                                                    }
                                                } else if (line.toLowerCase().startsWith("content-length:")) {
                                                    String t[] = line.split(" ");
                                                    contentLength = Integer.valueOf(t[1]);
                                                }
                                            } while (! done);

                                            status = 201;
                                        } else {
                                            status = 404;
                                        }
                                        break;

                                    case "/perm":
                                        session = getSession(query);
                                        if (session != null) {
                                            String line;
                                            int contentLength = 0;
                                            boolean done = false;
                                            do {
                                                line = in.readLine();
                                                if (line.equals("")) {
                                                    try (BufferedWriter out = new BufferedWriter(new FileWriter(new File(store, session.permfile)))) {
                                                        for (int i = 0; i < contentLength; i++) {
                                                            char c = (char)in.read();
                                                            out.append(c);
                                                        }
                                                        done = true;
                                                    }
                                                } else if (line.toLowerCase().startsWith("content-length:")) {
                                                    String t[] = line.split(" ");
                                                    contentLength = Integer.valueOf(t[1]);
                                                }
                                            } while (! done);

                                            session.run(query);

                                            status = 201;
                                        } else {
                                            status = 404;
                                        }
                                        break;
                                }
                                break;
                                
                            default:
                                Logger.log("Unknown method: %s", method);
                                status = 404;
                        }
                    } catch (URISyntaxException e) {
                        Logger.log("Invalid URI syntax: %s", request);
                        status = 404;
                    }

                    try (PrintWriter out = new PrintWriter(socket.getOutputStream(), true)) {
                        out.print("HTTP/1.1 ");
                        out.print(status);
                        out.printf(" ");

                        switch (status) {
                            case 100:
                                out.println("Continue");
                                break;

                            case 201:
                                out.println("Created");
                                break;

                            case 200:
                                out.println("OK");
                                break;

                            case 404:
                                out.println("Not Found");
                                break;

                            default:
                                out.println();
                        }

                        if (html) {
                            out.print("Content-type: text/html\r\n");
                        } else {
                            out.print("Content-Type: text/html; charset=iso-8859-1\r\n");
                        }
                        out.print("Server-name: UWECSP\r\n");
                        out.print("Content-length: ");
                        out.print(response.length());
                        out.print("\r\n");
                        out.print("\r\n");
                        out.print(response);
                        out.flush();
                        
                        if (response.length() < 64) {
                            Logger.log("Reply: %s", response);
                        } else {
                            Logger.log("Reply length: %d", response.length());
                        }
                    }
                } else {
                    Logger.log("Disconnect");
                }
            }
        } catch (IOException e) {
            Logger.log("Failed to respond to client request: %s", e.getMessage());
            System.exit(1);
        }
    }

}
