Product product = new Product("Milk", 6.5);

List<Product> products = new List<Product>();
products.Add(new Product("candle", 7.77));
products.Add(product);

product.SetPrice(-10);
product.SetPrice(10);

foreach (Product p in products)
{
    Console.WriteLine($"{p.GetName2()}: {p.GetPrice()}");
}