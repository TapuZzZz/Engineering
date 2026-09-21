import java.util.Scanner;

public class Main {
    public static void main (String[] args){

    Scanner scanner = new Scanner (System.in);

    System.out.print("Enter Your Bill: ");
    double bill = scanner.nextDouble();

    System.out.print("Enter Your Tip Amount (%): ");
    double tip = scanner.nextDouble();

    tip = bill * (tip / 100);
    double totalPrice = tip + bill;

    System.out.println("Your Tip Is " + tip + " ILS");
    System.out.println("Your Total Price To Pay Is " + totalPrice + " ILS");

    scanner.close();

    }
}