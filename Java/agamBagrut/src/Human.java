public class Human {
    private String name;
    private int id;

    public Human(String name, int id) {
        this.name = name;
        this.id = id;
    }

    public Human(int id) {
        this.name = "Unknown";
        this.id = id;
    }

    public String setName(String name) {
        if (name.length() < 5) {
            return "Name must be at least 5 characters long.";
        }
        this.name = name;
        return this.name;
    }

    public String getName(int code) {
        if (code != 1234) {
            return "Invalid code.";
        }
        return name;
    }

    public int getId() {
        return id;
    }
}   