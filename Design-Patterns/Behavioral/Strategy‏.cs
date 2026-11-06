public interface IDiscount { 
    double Apply(double price); 
}
public class NoDiscount : IDiscount { 
    public double Apply(double p) => p; 
}
public class HalfPrice : IDiscount { 
    public double Apply(double p) => p / 2; 
}

public class Cart
{
    public IDiscount Discount { get; set; } = new NoDiscount();
    public double Total(double price) => Discount.Apply(price);
}