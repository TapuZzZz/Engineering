public class Cart
{
    private int Quantity;
    private double FullPrice;
    private List<Product> ProductsList = new List<Product>();  // את חייבת להבין פה כל מילה בפני עצמה

    public int GetQuantity()
    {
        return Quantity;
    }

    public void SetQuantity(int quantity)
    {
        Quantity = quantity;
    }

    public double GetFullPrice()
    {
        return FullPrice;
    }

    public void SetFullPrice(double fulllPrice)
    {
        FullPrice = fulllPrice;
    }

    public List<Product> GetProductsList()
    {
        return ProductsList;
    }

    public void AddProduct(Product product)
    {
        // תחשבי מה צריך לעדכן בעת קבלת מוצר חדש לעגלה, ספויילר זה 3 שורות קוד יצא לי שזה הגיוני כי כמה תכונות יש למחלקה
    }

    public void RemoveProduct(Product product)
    {
        // תחשבי מה צריך לעדכן בעת הסרת מוצר מהעגלה, ספויילר זה 3 שורות קוד יצא לי שזה הגיוני כי כמה תכונות יש למחלקה
    }

}