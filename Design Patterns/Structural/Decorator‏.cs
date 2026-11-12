public interface IOutput { 
    void Write(string msg); 
}
public class ConsoleOutput : IOutput
{
    public void Write(string msg) => Console.WriteLine(msg);
}

public class TimeDecorator : IOutput
{
    private readonly IOutput _inner;
    public TimeDecorator(IOutput inner) { _inner = inner; }
    public void Write(string msg) =>
        _inner.Write($"[{DateTime.Now:HH:mm}] {msg}");
}