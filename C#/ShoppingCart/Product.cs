public class Product 
{
    private string Name;
    private double Price;

    public Product(string name, double price)
    {
        Name = name;
        Price = price;
    }

    public string GetName()
    {
        return Name;
    }

    public double GetPrice()
    {
        return Price;
    }

    public void SetPrice(double price)
    {
        if (price < 0)
        {
            System.Console.WriteLine("Price cannot be negative!");
            return;
        }
        Price = price;
    }
}