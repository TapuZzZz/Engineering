public class Doctor extends Human {
    private String specialization;
    
    public Doctor(String name, int id, String specialization) {
        super(name, id);
        this.specialization = specialization;
    }

    public Doctor(int id, String specialization) {
        super(id);
        this.specialization = specialization;
    }

    public String getSpecialization() {
        return specialization;
    }
}
