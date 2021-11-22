package client;

import java.io.*;
import java.net.Socket;
import java.util.Scanner;
import java.util.StringTokenizer;
import java.util.Vector;

public class Client {

    private Socket clientSocket = null;
    private DataInputStream dis = null;
    private DataOutputStream dos = null;

    public void sendFile(String fileName, String fileType, int CHUNK_SIZE,DataOutputStream dos) throws IOException {

        File file = new File("src/client/"+fileName);
        FileInputStream fis = new FileInputStream(file);

        BufferedInputStream bis = new BufferedInputStream(fis);
        byte[] contents;
        long fileLength = file.length();

        dos.writeUTF("file "+ fileLength +" "+fileName+" "+fileType);
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

    public void recieveFile(String fileName, String fileType,int filesize,DataInputStream dis) throws IOException {
        byte[] contents = new byte[10000];

        if(filesize!=0)
        {
            FileOutputStream fos = new FileOutputStream("src/client/"+fileType+"_"+fileName);
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


    Client()
    {
        try{
            clientSocket = new Socket("localhost", 6788);
            dis = new DataInputStream(clientSocket.getInputStream());
            dos = new DataOutputStream(clientSocket.getOutputStream());
        }catch(Exception e){
            System.out.println("Problem in connecting server, Exiting Main");
            System.exit(1);
        }

        Thread listenFromServer = new Thread(new Runnable() {
            @Override
            public void run() {

                while (true)
                {
                    try {
                        String textFromServer = dis.readUTF();
                        System.out.println("From Server => " + textFromServer);

                        StringTokenizer stringTokenizer = new StringTokenizer(textFromServer," ");
                        Vector<String> tokens = new Vector<>();

                        while (stringTokenizer.hasMoreTokens())
                        {
                            tokens.add(stringTokenizer.nextToken());
                        }

                        try {

                            if(tokens.elementAt(0).equals("f"))
                            {

                                int CHUNK_SIZE = Integer.parseInt(tokens.elementAt(1));
                                String fileId = tokens.elementAt(2);
                                String fileName = tokens.elementAt(3);
                                String fileType = tokens.elementAt(4);

                                sendFile(fileName,fileType,CHUNK_SIZE,dos);

                            }
                            else if(tokens.elementAt(0).equals("file"))
                            {
                                int filesize = Integer.parseInt(tokens.elementAt(1));
                                String fileName = tokens.elementAt(2);
                                String fileType = tokens.elementAt(3);

                                recieveFile(fileName,fileType,filesize,dis);
                                System.out.println("File downloaded");
                            }
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

                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }

            }
        });

        Thread consoleStream = new Thread(new Runnable() {

            public void run() {
                while (true) {

                    System.out.println("Console Stream : ");

                    Scanner sc = new Scanner(System.in);
                    String reply = sc.nextLine();

                    try {
                        dos.writeUTF(reply);
                        dos.flush();

                        if(reply.equals("exit"))
                        {
                            System.exit(1);
                        }

                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                }
            }
        });

        listenFromServer.start();
        consoleStream.start();
    }

    public static void main(String args[]) throws IOException {

        Client client = new Client();
    }
}
