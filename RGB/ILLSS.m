function rho=ILLSS(T,sRGB)
% This is the Iterative Least Log Slope Squared (ILLSS) algorithm for generating
% a "reasonable" reflectance curve from a given sRGB color triplet.
% The reflectance spans the wavelength range 380-730 nm in 10 nm increments.

% It solves min sum(z_i+1 - z_i)^2 s.t. T exp(z) = rgb, K z = 0, where
% z=log(reflectance), using Lagrangian approach and Newton's method.
% Clips values >1 and repeats optimization until all reflectance <=1.

% T is 3x36 matrix converting reflectance to linear rgb over the range 380-730 nm,
% sRGB is a 3 element vector of target D65 referenced sRGB values in 0-255 range,
% rho is a 36x1 vector of reflectance values (0->1] over wavelengths 380-730 nm,

% Written by Scott Allen Burns, 4/11/15.
% Licensed under a Creative Commons Attribution-ShareAlike 4.0 International
% License (http://creativecommons.org/licenses/by-sa/4.0/).
% For more information, see http://www.scottburns.us/subtractive-color-mixture/

% initialize output to zeros
rho=zeros(36,1);

% handle special case of (0,0,0)
if all(sRGB==0)
    rho=0.0001*ones(36,1);
    return
end

% handle special case of (255,255,255)
if all(sRGB==255)
    rho=ones(36,1);
    return
end

% 36x36 difference matrix having 4 on main diagonal and -2 on off diagonals,
% except first and last main diagonal are 2.
D=full(gallery('tridiag',36,-2,4,-2));
D(1,1)=2;
D(36,36)=2;

% compute target linear rgb values
sRGB=sRGB(:)/255; % convert to 0-1 column vector
rgb=zeros(3,1);
% remove gamma correction to get linear rgb
for i=1:3
    if sRGB(i)<0.04045
        rgb(i)=sRGB(i)/12.92;
    else
        rgb(i)=((sRGB(i)+0.055)/1.055)^2.4;
    end
end

% outer iteration to get all refl <=1
maxouter=10;
outer_count=0; % counter for outer iteration
while (any(rho>1) && outer_count<=maxouter) || all(rho==0)
    
    % create K matrix for fixed refl constraints
    fixed_refl=find(rho>=1)';
    numfixed=length(fixed_refl);
    K=zeros(numfixed,36);
    for i=1:numfixed
        K(i,fixed_refl(i))=1;
    end
    
    % initialize
    z=zeros(36,1); % starting point all zeros
    lambda=zeros(3,1); % starting point for lambda
    mu=zeros(numfixed,1); % starting point for mu
    maxit=50; % max number of iterations
    ftol=1.0e-8; % function solution tolerance
    deltatol=1.0e-8; % change in oper pt tolerance
    count=0; % iteration counter
    
    % Newton's method iteration
    while count <= maxit
        r=exp(z);
        v=-diag(r)*T'*lambda; % 36x1
        m1=-T*r; % 3x1
        m2=-T*diag(r); % 3x36
        F=[D*z+v+K'*mu;m1+rgb;K*z]; % function vector
        J=[D+diag(v),[m2',K'];[m2;K],zeros(numfixed+3)]; % Jacobian matrix
        delta=J\(-F); % solve Newton system of equations J*delta = -F
        z=z+delta(1:36); % update z
        lambda=lambda+delta(37:39); % update lambda
        mu=mu+delta(40:end);
        if all(abs(F)<ftol) % check if functions satisfied
            if all(abs(delta)<deltatol) % check if variables converged
                % solution found
                disp(['Inner loop solution found after ',num2str(count),' iterations'])
                rho=exp(z);
                break
            end
        end
        count=count+1;
    end
    if count>=maxit
        disp(['No inner loop solution found after ',num2str(maxit),' iterations.'])
    end
    outer_count=outer_count+1;
end
if outer_count<maxouter
    disp(['Outer loop solution found after ',num2str(outer_count),' iterations'])
else
    disp(['No outer loop solution found after ',num2str(maxouter),' iterations.'])
end
