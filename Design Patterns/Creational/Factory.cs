public interface IAnimal { 
    string Sound(); 
}
public class Dog : IAnimal { 
    public string Sound() => "Woof"; 
}
public class Cat : IAnimal { 
    public string Sound() => "Meow"; 
}

public static class AnimalFactory
{
    public static IAnimal Create(string type) => type == "dog" ? new Dog() : new Cat();
}