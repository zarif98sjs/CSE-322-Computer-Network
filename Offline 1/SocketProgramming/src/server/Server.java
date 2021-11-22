package server;

import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Random;
import java.util.StringTokenizer;
import java.util.Vector;

public class Server {

    long MAX_BUFFER_SIZE = 1000000000;
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
        int diff = b-a+1;
        Random rd = new Random();
        return a + (Math.abs(rd.nextInt()) % diff);
    }

    public String getFileId(User u)
    {
        return Integer.toString(u.getId()) + "_"+Integer.toString(u.getFileCounter());
    }

    public void recieveFile(String fileName, String fileType,int filesize,int userID,DataInputStream dis) throws IOException {
        byte[] contents = new byte[10000];

        if(filesize!=0)
        {
            FileOutputStream fos = new FileOutputStream("files/"+userID+"/"+fileType+"/"+fileName);
            BufferedOutputStream bos = new BufferedOutputStream(fos);

            int bytesRead = 0;
            int total=0;

            while(total!=filesize)
            {
                bytesRead=dis.read(contents);
                total+=bytesRead;
                System.out.println("Current read (total) : "+total);
                bos.write(contents, 0, bytesRead);
            }
            bos.flush();
        }
    }

    public void sendFile(String fileName, String fileType,int uID,int CHUNK_SIZE,DataOutputStream dos) throws IOException {

        File file = new File("files/"+uID+"/"+fileType+"/"+fileName);

        FileInputStream fis = new FileInputStream(file);

        BufferedInputStream bis = new BufferedInputStream(fis);
        byte[] contents;
        long fileLength = file.length();

        dos.writeUTF("file "+Long.toString(fileLength)+" "+fileName+" "+fileType);
        dos.flush();

        long current = 0;

        while(current!=fileLength){
            System.out.println("Current "+current);
            int size = CHUNK_SIZE;
            if(fileLength - current >= size)
                current += size;
            else{
                size = (int)(fileLength - current);
                current = fileLength;
            }
            contents = new byte[size];
            bis.read(contents, 0, size);
//                                    System.out.println(contents);
            dos.write(contents);
            dos.flush();
        }
        dos.flush();
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

        while(true) {

            Socket connectionSocket = welcomeSocket.accept();

            DataInputStream dis = new DataInputStream(connectionSocket.getInputStream());
            DataOutputStream dos = new DataOutputStream(connectionSocket.getOutputStream());

            User curUser = new User(connectionSocket.getPort(),dis,dos);
            System.out.println("Just connected to client with port -> " + connectionSocket.getPort());

            Thread workerThread = new Thread(new Runnable() {

                @Override
                public void run() {

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
                            else if(tokens.elementAt(0).equals("b_d"))
                            {
                                // download own file request from user : file_type file_name

                                String fileType = tokens.elementAt(1);
                                String fileName = tokens.elementAt(2);

                                sendFile(fileName,fileType,curUser.getId(),50, curUser.getDos());

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
                            else if(tokens.elementAt(0).equals("f"))
                            {
                                // from user : f file_name file_type
                                // Upload file

                                String fileName = tokens.elementAt(1);
                                String fileType = tokens.elementAt(2);
                                File file = new File("src/com/company/"+fileName);
                                long fileSize = file.length();
                                System.out.println("Size of the file : "+fileSize);

                                if(CUR_BUFFER_SIZE + fileSize <= MAX_BUFFER_SIZE)
                                {
                                    curUser.write("f " + getRandom(MIN_CHUNK_SIZE, MAX_CHUNK_SIZE) + " "+ getFileId(curUser) + " " + fileName + " " + fileType);
                                }
                                else
                                {

                                }
                            }
                            else if(tokens.elementAt(0).equals("file"))
                            {
                                try
                                {
                                    int filesize = Integer.parseInt(tokens.elementAt(1));
                                    String fileName = tokens.elementAt(2);
                                    String fileType = tokens.elementAt(3);

                                    recieveFile(fileName,fileType,filesize,curUser.getId(),dis);

                                    curUser.write("File Uploaded");

                                }
                                catch(Exception e)
                                {
                                    System.err.println("Could not transfer file.");
                                }
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
