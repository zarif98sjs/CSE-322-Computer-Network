package server;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.Vector;

public class User {

    int id;
    int clientSocketID;
    private DataInputStream dis;
    private DataOutputStream dos;
    boolean isOnline;
    int fileCounter;

    Vector<Message> messageVector;

    public User(int clientSocketID,DataInputStream dis, DataOutputStream dos) {
        this.clientSocketID = clientSocketID;
        this.dis = dis;
        this.dos = dos;
        this.isOnline = true;
        this.fileCounter = 0;
        this.messageVector = new Vector<Message>();
    }

    public int getClientSocketID()
    {
        return clientSocketID;
    }

    public void askId() throws IOException {
        dos.writeUTF("Welcome to the Hive . Please type your ID : ");
        dos.flush();
    }

    public void setId(int id) throws IOException {
        this.id = id;
        System.out.println("Client ID : " + id);

        dos.writeUTF("Congrats! You are now in.");
        dos.flush();

        // make folder for this user
        new File("files/"+Integer.toString(id)+"/public").mkdirs();
        new File("files/"+Integer.toString(id)+"/private").mkdirs();
    }

    public int getId()
    {
        return id;
    }

    public DataOutputStream getDos() {
        return dos;
    }

    public void setDos(DataOutputStream dos) {
        this.dos = dos;
    }

    public void write(String text) throws IOException {
        dos.writeUTF(text);
        dos.flush();
    }

    public void makeOffline()
    {
        isOnline = false;
    }

    public String getStatus()
    {
        if(isOnline) return "Online";
        else return "Offline";
    }

    public int getFileCounter() {
        return fileCounter;
    }

    public void addMessageToQueue(Message message)
    {
        messageVector.add(message);
    }

    public Vector<String> showUnreadMessage()
    {
        System.out.println("Showing unread messages ...");
        Vector<String>stringVector = new Vector<String>();
        for(Message m:messageVector)
        {
            System.out.println(m.isRead());
            if(!m.isRead())
            {
                stringVector.add("From "+m.getFrom()+" : "+m.getMessage());
                m.setRead(true);
            }
        }
        return stringVector;
    }


}
