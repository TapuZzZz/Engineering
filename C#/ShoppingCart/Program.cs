Product product = new Product("Milk", 6.5);

List<Product> products = new List<Product>();
products.Add(new Product("candle", 7.77));
products.Add(product);

product.SetPrice(-10);
product.SetPrice(10);

foreach (Product p in products)
{
    Console.WriteLine($"{p.GetName()}: {p.GetPrice()}");
}

// פה בבקשה תמחקי את כל הקוד לגמרי ואני רוצה שתייצרי עגלה אחת תוסיפי לה 5 מוצרים 
// אחרי זה אני רוצה שתרוצי בלולאה של פור רגיל ואחרי זה בנפרד בפור איץ ואני רוצה שתדפיסי את כל הפריטים 
// אחרי זה אני רוצה שתורידי בקארט 2 מוצרים 
// תדפיסי את מה שנשאר בעגלה
// ותחשבי איך את מחשבת את המחיר הסופי כולל 18 אחוז מס 
// o(1) - ב 
// כלומר אסור לולאות 