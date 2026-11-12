public interface ITempSensor { 
    float GetTemperature(); 
}
public class LibSensor { 
    public float ReadTemp() => 25f; 
}

public class SensorAdapter : ITempSensor
{
    private readonly LibSensor _lib;
    public SensorAdapter(LibSensor lib) { _lib = lib; }
    public float GetTemperature() => _lib.ReadTemp();
}