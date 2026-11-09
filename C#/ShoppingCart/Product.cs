public class Product 
{
    private string Name;
    private double Price;

    public Product(string name, double price)
    {
        Name = name;
        Price = price;
    }

    private string GetName()
    {
        return this.Name;
    }

    public string GetName2()
    {
        return GetName();
    }

    public double GetPrice()
    {
        return this.Price;
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