public class Cart
{
    private int Quantity;
    private double FullPrice;
    private List<Product> ProductsList = new List<Product>();

    public int GetQuantity()
    {
        return this.Quantity;
    }

    public void SetQuantity(int quantity)
    {
        Quantity = quantity;
    }

    public double GetFullPrice()
    {
        return this.FullPrice;
    }

    public void SetFullPrice(double fulllPrice)
    {
        FullPrice = fulllPrice;
    }

    public List<Product> GetProductsList()
    {
        return this.ProductsList;
    }

    public void SetProductsList(List<Product> productsList)
    {
        ProductsList = productsList;
    }


}