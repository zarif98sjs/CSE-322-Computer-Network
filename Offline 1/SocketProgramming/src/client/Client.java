package client;

import java.io.*;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.Scanner;
import java.util.StringTokenizer;
import java.util.Vector;

public class Client {

    private Socket clientSocket = null;
    private DataInputStream dis = null;
    private DataOutputStream dos = null;

    private Socket clientSocketFile = null;
    private DataInputStream disFile = null;
    private DataOutputStream dosFile = null;

    public void sendFile(String fileName, String fileType, int CHUNK_SIZE,DataInputStream dataInputStream,DataOutputStream dataOutputStream,DataInputStream dataInputStreamFile,DataOutputStream dataOutputStreamFile) throws IOException {

        File file = new File("src/client/"+fileName);
        FileInputStream fileInputStream = new FileInputStream(file);

        long fileLength = file.length();

        dataOutputStreamFile.writeUTF("file "+ fileLength +" "+fileName+" "+fileType+" "+CHUNK_SIZE);
        dataOutputStreamFile.flush();

        // break file into chunks
        int bytes = 0;
        byte[] buffer = new byte[CHUNK_SIZE];
        int CHUNK = 0;
        while ((bytes=fileInputStream.read(buffer))!=-1){

//            System.out.println("Chunk #"+CHUNK); CHUNK++;

            dataOutputStreamFile.write(buffer,0,bytes);
            dataOutputStreamFile.flush();

            try {
                // ACK
                String msg = dataInputStreamFile.readUTF();
                if(!msg.equals("ACK"))
                {
                    System.out.println("Did not receive ACK...");
                    break;
                }

            }catch (SocketTimeoutException socketTimeoutException){
                System.out.println("TIMEOUT");
                dataOutputStreamFile.writeUTF("TIMEOUT "+fileType+" "+fileName);
                dataOutputStreamFile.flush();
                fileInputStream.close();
                return;
            }
        }
        fileInputStream.close();

        // send confirmation
        dataOutputStreamFile.writeUTF("ACK");
        dataOutputStreamFile.flush();

        String msg = dataInputStreamFile.readUTF();
        if(msg.equals("ACK")) System.out.println("File Upload Completed");
        else System.out.println("File Upload Failed");

    }

    public void recieveFile(String fileName, String fileType,int filesize,DataInputStream dataInputStream,int CHUNK_SIZE) throws IOException {

        int bytes = 0;
        FileOutputStream fileOutputStream = new FileOutputStream("src/client/"+fileType+"_"+fileName);

        int size = filesize;     // read file size
        byte[] buffer = new byte[CHUNK_SIZE];
        int CHUNK = 0;
        while (size > 0 && (bytes = dataInputStream.read(buffer, 0, Math.min(buffer.length, size))) != -1) {
//            System.out.println("Chunk #"+CHUNK);
            CHUNK++;
            fileOutputStream.write(buffer,0,bytes);
            size -= bytes;      // read upto file size

        }
        fileOutputStream.close();
    }


    Client()
    {
        try{
            clientSocket = new Socket("localhost", 6788);
            dis = new DataInputStream(clientSocket.getInputStream());
            dos = new DataOutputStream(clientSocket.getOutputStream());

            clientSocketFile = new Socket("localhost", 6789);
            disFile = new DataInputStream(clientSocketFile.getInputStream());
            dosFile = new DataOutputStream(clientSocketFile.getOutputStream());


        }catch(Exception e){
            System.out.println("Problem in connecting server, Exiting Main");
            System.exit(1);
        }

        Thread listenFromServer = new Thread(new Runnable() {
            @Override
            public void run() {

                // first
                String textFromServer = null;
                try {
                    textFromServer = dis.readUTF();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                System.out.println("From Server => " + textFromServer);

                while (true)
                {
                    // ping
                    System.out.println("Console ...");
                    Scanner sc = new Scanner(System.in);
                    String reply = sc.nextLine();
                    System.out.println("Reply : "+reply);
                    StringTokenizer stringTokenizer = new StringTokenizer(reply," ");
                    Vector<String>tokens = new Vector<>();

                    while (stringTokenizer.hasMoreTokens())
                    {
                        tokens.add(stringTokenizer.nextToken());
                    }

                    try {

                        if(tokens.elementAt(0).equals("f") || tokens.elementAt(0).equals("b_d"))
                        {
                            dosFile.writeUTF(reply);
                            dosFile.flush();
                        }
                        else{
                            dos.writeUTF(reply);
                            dos.flush();
                        }

                        if(reply.equals("exit"))
                        {
                            System.exit(1);
                        }

                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                    try {
                        // ping reply
                        textFromServer = dis.readUTF();
                        System.out.println("From Server => " + textFromServer);

//                        stringTokenizer = new StringTokenizer(textFromServer," ");
//                        tokens = new Vector<>();
//
//                        while (stringTokenizer.hasMoreTokens())
//                        {
//                            tokens.add(stringTokenizer.nextToken());
//                        }

                        //                            if(tokens.elementAt(0).equals("f"))
//                            {
//
//                                int CHUNK_SIZE = Integer.parseInt(tokens.elementAt(1));
//                                String fileId = tokens.elementAt(2);
//                                String fileName = tokens.elementAt(3);
//                                String fileType = tokens.elementAt(4);
//
//                                sendFile(fileName,fileType,CHUNK_SIZE,dis,dos,disFile,dosFile);
//
//                            }
//                            else if(tokens.elementAt(0).equals("file"))
//                            {
//                                int filesize = Integer.parseInt(tokens.elementAt(1));
//                                String fileName = tokens.elementAt(2);
//                                String fileType = tokens.elementAt(3);
//                                int CHUNK_SIZE = Integer.parseInt(tokens.elementAt(4));
//
//                                recieveFile(fileName,fileType,filesize,dis,CHUNK_SIZE);
//                                System.out.println("File downloaded");
//                            }
//                            else
//                            {
//                                Scanner sc = new Scanner(System.in);
//                                String reply = sc.nextLine();
//                                dos.writeUTF(reply);
//                                dos.flush();
//
//                                if(reply.equals("exit"))
//                                {
//                                    System.exit(1);
//                                }
//
//                            }

                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }

            }
        });

        Thread fileStream = new Thread(new Runnable() {

            public void run() {
                while (true) {

                    System.out.println("File Stream : ");

                    // ping reply
                    String textFromServerFile = null;
                    try {
                        textFromServerFile = disFile.readUTF();
                        System.out.println("From Server (File Stream) => " + textFromServerFile);

                        StringTokenizer stringTokenizer = new StringTokenizer(textFromServerFile," ");
                        Vector<String>tokens = new Vector<>();

                        while (stringTokenizer.hasMoreTokens())
                        {
                            tokens.add(stringTokenizer.nextToken());
                        }

                        if(tokens.elementAt(0).equals("f"))
                        {

                            int CHUNK_SIZE = Integer.parseInt(tokens.elementAt(1));
                            String fileId = tokens.elementAt(2);
                            String fileName = tokens.elementAt(3);
                            String fileType = tokens.elementAt(4);

                            clientSocketFile.setSoTimeout(5000);
                            sendFile(fileName,fileType,CHUNK_SIZE,dis,dos,disFile,dosFile);
                            clientSocketFile.setSoTimeout(0);
                        }
                        else if(tokens.elementAt(0).equals("file"))
                        {
                            int filesize = Integer.parseInt(tokens.elementAt(1));
                            String fileName = tokens.elementAt(2);
                            String fileType = tokens.elementAt(3);
                            int CHUNK_SIZE = Integer.parseInt(tokens.elementAt(4));

                            recieveFile(fileName,fileType,filesize,disFile,CHUNK_SIZE);
                            System.out.println("File downloaded");
                        }

                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                }
            }
        });

        listenFromServer.start();
        fileStream.start();
    }

    public static void main(String args[]) throws IOException {

        Client client = new Client();
    }
}
