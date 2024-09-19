# Analysis for its-only residual

## Circular Fit in the XY plane : circleFitXY

We use the Weighted Least Square(WLS) fit and calculate the residuals from each cluster positions to the fitted trajectory. 

In the XY plane, the trajectory of particle under the magnetic field in z-axis can be fitted with circular model.
There are 4 fit parameters : radius(R), direction at origin(tR), estimated vertex X(vx), and estimated vertex Y(vy).

xc = R > 0 ? R*std::cos(tR + 0.5*TMath::Pi()) : R*std::cos(tR - 0.5*TMath::Pi())
yc = R > 0 ? R*std::sin(tR + 0.5*TMath::Pi()) : R*std::sin(tR - 0.5*TMath::Pi())
(x - (xc + vx))^2 + (y - (yc + vy))^2 = |R|^2

For the fit stability, 
1. A simple algorithm to determine seeds of fit parameters(radius and direction at origin) is applied.
2. Circular fitting is performed for both (+) and (-) signs of the circular trajectory (even if we have approximated seeds from 1.), the one with the lower chi2(sum of weighted residuals) is finally adopted. This procedure ensures the stability in case of hard(high pT) tracks. 

## Linear Fit in the DZ plane : lineFitDZ

After 4 fit parameters of circular trajectory is determined, we can describe the fit positions of associated clusters as well as the global positions.
This makes it possible to represent each cluster position in the poloar coordinates(r, beta), whose origin is (xc, yc).

One of the simplest residual in z direction can be calculated in the RZ plane, where r = sqrt(x*x + y*y).
However, in this case, the fitting stability cannot be guaranteed if the collision position is not located in the same quadrant.

On the other hand, another parameter beta determined above the equation of the circle determined (in the XY plane) corresponds to the distance of the particle moving(=D) in the XY plane.
In this case, the fitting stability can be guaranteed even if the collision position is not located in the same quadrant.

So we proceed with the linear fit in the DZ plane.

