class Locator {
public:
    explicit Locator();
    void locate();
    void setXDistance(double distance);
    void setAngle(int *angle);
    void setIntensity(int *intensity);
    void setRange(int *range);
    void setZDistance(double distance);
private:
    void tcpServer(double *x, double *y);
};
