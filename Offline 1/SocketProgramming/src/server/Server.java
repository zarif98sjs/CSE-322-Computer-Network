package server;

import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.Random;
import java.util.StringTokenizer;
import java.util.Vector;

public class Server {

    long MAX_BUFFER_SIZE = 30;
    int MIN_CHUNK_SIZE = 10;
    int MAX_CHUNK_SIZE = 50;

    long CUR_BUFFER_SIZE = 0;

    Vector<User> users = new Vector<>();

    public User findUser(int id)
    {
        for(User u:users)
        {
            if(u.getId() == id)
                return u;
        }

        return null;
    }

    public void removeUser(int id)
    {
        int idx = 0;
        for(User u:users)
        {
            if(u.getId() == id)
            {
                users.remove(idx);
            }
            idx++;
        }
    }

    public String[] lookupPublicFiles(int uID){
        File directoryPath = new File("files/"+uID+"/public");
        //List of all files and directories
        String contents[] = directoryPath.list();
        return contents;
    }

    public String[] lookupPrivateFiles(int uID){
        File directoryPath = new File("files/"+uID+"/private");
        //List of all files and directories
        String contents[] = directoryPath.list();
        return contents;
    }


    public int getRandom(int a,int b) // get a random int in [a,b]
    {
        return 25;
//        int diff = b-a+1;
//        Random rd = new Random();
//        return a + (Math.abs(rd.nextInt()) % diff);
    }

    public String getFileId(User u)
    {
        return Integer.toString(u.getId()) + "_"+Integer.toString(u.getFileCounter());
    }

    public boolean recieveFile(String fileName, String fileType,int filesize,int userID,DataInputStream dataInputStreamFile,DataOutputStream dataOutputStreamFile,int CHUNK_SIZE) throws IOException {

        int bytes = 0;
        FileOutputStream fileOutputStream = new FileOutputStream("files/"+userID+"/"+fileType+"/"+fileName);

        int size = filesize;     // read file size
        byte[] buffer = new byte[CHUNK_SIZE];
        int CHUNK = 0;
        // extra
        CUR_BUFFER_SIZE += CHUNK_SIZE;
        while (size > 0) {

            boolean ok;
            try {
                ok = (bytes = dataInputStreamFile.read(buffer, 0, Math.min(buffer.length, size))) != -1;
            }catch (SocketTimeoutException socketTimeoutException){
                CUR_BUFFER_SIZE -= CHUNK_SIZE;
                fileOutputStream.close();
                System.out.println("FileOutputStream Closed");
                return false;
            }

            if(!ok) break;

            if(CHUNK % 10000 == 0)System.out.println("Chunk #"+CHUNK);
            CHUNK++;

//            if(CHUNK != 1) // hardcode to check file size difference
                fileOutputStream.write(buffer,0,bytes);

            size -= bytes;      // read upto file size
            // send ACK
            if(CHUNK <= 1) { // hardcode timeout
                dataOutputStreamFile.writeUTF("ACK");
                dataOutputStreamFile.flush();
            }

        }

        CUR_BUFFER_SIZE -= CHUNK_SIZE;
        fileOutputStream.close();
        System.out.println("FileOutputStream Closed");

        // check confirmation and validate file size
        String msg = dataInputStreamFile.readUTF();
        if(msg.equals("ACK")){
            File file = new File("files/"+userID+"/"+fileType+"/"+fileName);
            if(file.length() != filesize)
            {
                System.out.println("File size mismatch");
                file.delete();
                return false;
            }
        }
        else
        {
            File file = new File("files/"+userID+"/"+fileType+"/"+fileName);
            file.delete();
            return false;
        }

        return true;
    }

    public void sendFile(String fileName, String fileType,int uID,int CHUNK_SIZE,DataOutputStream dataOutputStream) throws IOException {

        File file = new File("files/"+uID+"/"+fileType+"/"+fileName);
        FileInputStream fileInputStream = new FileInputStream(file);

        long fileLength = file.length();

        dataOutputStream.writeUTF("file "+ fileLength +" "+fileName+" "+fileType+" "+CHUNK_SIZE);
        dataOutputStream.flush();

        // break file into chunks
        int bytes = 0;
        byte[] buffer = new byte[CHUNK_SIZE];
        int CHUNK = 0;
        while ((bytes=fileInputStream.read(buffer))!=-1){
            if(CHUNK % 10000 == 0) System.out.println("Chunk #"+CHUNK);
            CHUNK++;
            dataOutputStream.write(buffer,0,bytes);
            dataOutputStream.flush();
        }
        fileInputStream.close();
    }

//    public void updateUsers()
//    {
//        File directoryPath = new File("files");
//        //List of all files and directories
//        String contents[] = directoryPath.list();
//        for(String s:contents)
//        {
//            System.out.println(s);
//        }
//    }


    Server () throws IOException {

        ServerSocket welcomeSocket = new ServerSocket(6788);
//        welcomeSocket.setReceiveBufferSize((int) MAX_BUFFER_SIZE);
        ServerSocket welcomeSocketFile = new ServerSocket(6789);

        while(true) {

            Socket connectionSocket = welcomeSocket.accept();
            DataInputStream dis = new DataInputStream(connectionSocket.getInputStream());
            DataOutputStream dos = new DataOutputStream(connectionSocket.getOutputStream());

            Socket connectionSocketFile = welcomeSocketFile.accept();
//            connectionSocketFile.setSoTimeout(5000);
            DataInputStream disFile = new DataInputStream(connectionSocketFile.getInputStream());
            DataOutputStream dosFile = new DataOutputStream(connectionSocketFile.getOutputStream());

            User curUser = new User(connectionSocket.getPort(),dis,dos,disFile,dosFile);
            System.out.println("Just connected to client with port -> " + connectionSocket.getPort() + ", file : "+connectionSocketFile.getPort());

            Thread workerThread = new Thread(new Runnable() {

                @Override
                public void run() {

                    System.out.println("Server started");

                    try {
                        // ask login info
                        curUser.askId();
                        // get & set login info
                        String textFromClient = dis.readUTF();
                        int id = Integer.parseInt((textFromClient));
                        if(isAlreadyLoggedIn(id))
                        {
                            curUser.write("You are already logged in");
                        }
                        else if(hasLoggedInBefore(id))
                        {
                            removeUser(id);
                            curUser.setId(id);
                            users.add(curUser);
                        }
                        else
                        {
                            curUser.setId(id);
                            users.add(curUser);
                        }

                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                    while (true) {
                        String textFromClient;
                        try {

                            textFromClient = dis.readUTF();
                            System.out.println("Text from client {"+ curUser.getClientSocketID() + " , " + curUser.getId() +  "} : "+textFromClient);

                            StringTokenizer stringTokenizer = new StringTokenizer(textFromClient," ");
                            Vector<String>tokens = new Vector<>();

                            while (stringTokenizer.hasMoreTokens())
                            {
                                tokens.add(stringTokenizer.nextToken());
                            }

                            if(tokens.elementAt(0).equals("a"))
                            {
                                // Lookup Students : online and offline distinguishable
                                curUser.write(getUsers());
                            }
                            else if(tokens.elementAt(0).equals("b"))
                            {
                                String[] publicFiles = lookupPublicFiles(curUser.getId());
                                String ret = "Your Public Files : \n";
                                for(String s:publicFiles)
                                {
                                    ret += (s + "\n");
                                }

                                String[] privateFiles = lookupPrivateFiles(curUser.getId());
                                ret += "\nYour Private Files : \n";
                                for(String s:privateFiles)
                                {
                                    ret += (s + "\n");
                                }
                                curUser.write(ret);
                            }
                            else if(tokens.elementAt(0).equals("c"))
                            {
                                // lookup public files of a specific student

                                int userID = Integer.parseInt(tokens.elementAt(1));
                                String[] publicFiles = lookupPublicFiles(userID);
                                String ret = "Public Files of "+userID+" : \n";
                                for(String s:publicFiles)
                                {
                                    ret += (s + "\n");
                                }
                                curUser.write(ret);
                            }
                            else if(tokens.elementAt(0).equals("c_d"))
                            {
                                // download file request from user : student_id file_type file_name

                                int uID = Integer.parseInt(tokens.elementAt(1));
                                String fileType = tokens.elementAt(2);
                                String fileName = tokens.elementAt(3);

                                sendFile(fileName,fileType,uID,50, curUser.getDos());

                            }
                            else if(tokens.elementAt(0).equals("send"))
                            {
                                // send message to other user : send user_id message
                                System.out.println("sending message .. ");
                                int to = Integer.parseInt(tokens.elementAt(1));
                                String message = tokens.elementAt(2);
                                sendMessage(curUser.getId(),to,message);
                                curUser.write("Message sent successfully");
                            }
                            else if(tokens.elementAt(0).equals("show"))
                            {
                                System.out.println("Showinng");

                                Vector<String>stringVector =  curUser.showUnreadMessage();

                                String reply = "Unread Messages\n";
                                System.out.println(reply);

                                for(String s:stringVector)
                                {
                                    reply += s + "\n";
                                }

                                System.out.println("Reply : "+reply);

                                curUser.write(reply);

                            }
                            else if(tokens.elementAt(0).equals("TIMEOUT"))
                            {
                                String fileType = tokens.elementAt(1);
                                String fileName = tokens.elementAt(2);

                                File file = new File("files/"+curUser.getId()+"/"+fileType+"/"+fileName);
                                System.out.println(file.delete());
                                curUser.writeToFileStream("File Deleted");
                            }
                            else if(tokens.elementAt(0).equals("exit"))
                            {
                                curUser.makeOffline();
                                Thread.currentThread().interrupt(); // preserve the message
                                return;
                            }
                        } catch (Exception e) {
                            curUser.makeOffline();
                            Thread.currentThread().interrupt(); // preserve the message
                            return;
                        }
                    }
                }
            });

            workerThread.start();

            Thread workerThreadFile = new Thread(new Runnable() {

                @Override
                public void run() {

                    while (true)
                    {
                        try {
                            String textFromClient = disFile.readUTF();
                            System.out.println("Text from client (File) {"+ curUser.getClientSocketID() + " , " + curUser.getId() +  "} : "+textFromClient);

                            StringTokenizer stringTokenizer = new StringTokenizer(textFromClient," ");
                            Vector<String>tokens = new Vector<>();

                            while (stringTokenizer.hasMoreTokens())
                            {
                                tokens.add(stringTokenizer.nextToken());
                            }

                            if(tokens.elementAt(0).equals("b_d"))
                            {
                                // download own file request from user : file_type file_name

                                String fileType = tokens.elementAt(1);
                                String fileName = tokens.elementAt(2);

                                curUser.write("ok. download started");
                                sendFile(fileName,fileType,curUser.getId(),MAX_CHUNK_SIZE, curUser.getDosFile());

                            }
                            else if(tokens.elementAt(0).equals("f"))
                            {
                                // from user : f file_name file_type
                                // Upload file

                                String fileName = tokens.elementAt(1);
                                String fileType = tokens.elementAt(2);
                                File file = new File("src/com/company/"+fileName);
                                long fileSize = file.length();
                                int CHUNK_SIZE = getRandom(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE);
                                System.out.println("Size of the file : "+fileSize);
//                                System.out.println("Buffer Size : "+welcomeSocket.getReceiveBufferSize());
                                if(CUR_BUFFER_SIZE + CHUNK_SIZE <= MAX_BUFFER_SIZE)
                                {
                                    System.out.println("...");
                                    curUser.write("ok");
                                    curUser.writeToFileStream("f " + CHUNK_SIZE + " "+ getFileId(curUser) + " " + fileName + " " + fileType);
                                }
                                else
                                {
                                    System.out.println("Buffer size limit exceeded");
                                    curUser.write("not ok");
                                    curUser.writeToFileStream("Buffer size limit exceeded");
                                }
                            }
                            else if(tokens.elementAt(0).equals("file"))
                            {
                                try
                                {
                                    int filesize = Integer.parseInt(tokens.elementAt(1));
                                    String fileName = tokens.elementAt(2);
                                    String fileType = tokens.elementAt(3);
                                    int CHUNK_SIZE = Integer.parseInt(tokens.elementAt(4));

                                    if(recieveFile(fileName,fileType,filesize,curUser.getId(),disFile,dosFile,CHUNK_SIZE))
                                    {
                                        curUser.writeToFileStream("ACK");
                                        System.out.println("File Upload Completed");
                                    }
                                    else
                                    {
                                        curUser.writeToFileStream("NOT_ACK");
                                        System.out.println("File Upload Failed");
                                    }

                                }
                                catch(Exception e)
                                {
                                    System.err.println("Could not transfer file.");
                                }
                            }

                        } catch (Exception e) {
//                            e.printStackTrace();
                            curUser.makeOffline();
                            Thread.currentThread().interrupt(); // preserve the message
                            return;
                        }
                    }

                }
            });

            workerThreadFile.start();


        }
    }

    public void sendMessage(int from,int to,String msg){
        User toUser = findUser(to);
        System.out.println(toUser.getId());
        toUser.addMessageToQueue(new Message(from,to,msg));
//        System.out.println("Showing unread messages..");
//        toUser.showUnreadMessage();
    }


    public String getUsers()
    {
        String toPrint = "All Users : \n";

        for(User u:users)
        {
            toPrint += u.getId() + " ( " + u.getStatus() + " ) \n";
        }

        return toPrint;
    }

    public boolean isAlreadyLoggedIn(int id)
    {
        for(User u:users)
        {
            if(u.getId() == id && u.isOnline == true) return true;
        }

        return false;
    }

    public boolean hasLoggedInBefore(int id)
    {
        for(User u:users)
        {
            if(u.getId() == id && u.isOnline == false) return true;
        }

        return false;
    }



    public static void main(String[] args) throws IOException {

        Server server = new Server();

    }
}
