public class Button
{
    public event Action? Clicked;
    public void Press() => Clicked?.Invoke();
}