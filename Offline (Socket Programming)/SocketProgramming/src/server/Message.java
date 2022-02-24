package server;

public class Message {

    int from;
    int to;
    String message;
    boolean isRead;

    public Message(int from, int to, String message) {
        this.from = from;
        this.to = to;
        this.message = message;
        this.isRead = false;
    }

    public int getFrom() {
        return from;
    }

    public void setFrom(int from) {
        this.from = from;
    }

    public int getTo() {
        return to;
    }

    public void setTo(int to) {
        this.to = to;
    }

    public String getMessage() {
        return message;
    }

    public void setMessage(String message) {
        this.message = message;
    }

    public boolean isRead() {
        return isRead;
    }

    public void setRead(boolean read) {
        isRead = read;
    }
}
