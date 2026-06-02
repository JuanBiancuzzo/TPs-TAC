function Hz = backwards(Hs,T,output,normalize)
    type = 'backward';
    % defaults "output" to 'tf' if not input
    if (nargin < 4) || isempty(output)
        output = 'tf';
    end
    
    % defaults "normalize" to false if not input
    if (nargin < 5) || isempty(normalize)
        normalize = false;
    end
    
    % symbolic variable for z;
    z = sym('z');
    
    % specified Euler approximation of s
    if strcmpi(type,'backward')
        s = (z-1)/(T*z);
    elseif strcmpi(type,'forward')
        s = (z-1)/T;
    else
        error("'type' must be input as 'backward' or 'forward'")
    end
    
    % converts transfer function object to symbolic function object
    [num,den] = tfdata(Hs);
    Hz = poly2sym(cell2mat(num),z)/poly2sym(cell2mat(den),z);
    
    % performs Euler transformation
    Hz = simplify(subs(Hz,s));
    
    % obtains numerator and denominator of symbolic expression in MATLAB's
    % "polynomial form"
    [sym_num,sym_den] = numden(Hz);
    num = sym2poly(sym_num);
    den = sym2poly(sym_den);
    
    % normalizes coefficients w.r.t. coefficient on largest power of z in
    % denominator
    if normalize
        num = num/den(1);
        den = den/den(1);
    end
    
    % creates discrete transfer function model
    Hz = tf(num,den,T);
    
    % converts discrete transfer function model to discrete zero-pole-gain
    % model if specified
    if strcmpi(output,'zpk')
        Hz = zpk(Hz);
    end
    
end

