package client;

import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.control.Button;
import javafx.stage.FileChooser;
import javafx.stage.Stage;

import java.io.*;
import java.net.Socket;
import java.net.URL;
import java.util.ResourceBundle;
import java.util.StringTokenizer;
import java.util.Vector;

public class Controller implements Initializable {

    private Socket clientSocket = null;
    private DataInputStream dis = null;
    private DataOutputStream dos = null;

    public void sendFile(String fileName, String fileType, int CHUNK_SIZE,DataOutputStream dos) throws IOException {

        File file = new File("src/client/"+fileName);
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

    @FXML private Button uploadButton;
    @FXML private Button showStudents;

    public Controller() throws IOException {

    }

    @Override
    public void initialize(URL location, ResourceBundle resources) {

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

        listenFromServer.start();

        uploadButton.setOnAction(event -> {
            System.out.println("Click");
            FileChooser fileChooser = new FileChooser();
            fileChooser.setTitle("Open Resource File");
            File file  = fileChooser.showOpenDialog(new Stage());
            System.out.println(file);
        });

        showStudents.setOnAction(event -> {
            try {
                dos.writeUTF("a");
                dos.flush();
            } catch (IOException e) {
                e.printStackTrace();
            }
        });
    }
}
