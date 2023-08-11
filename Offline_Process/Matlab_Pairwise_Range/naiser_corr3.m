function [begin_idx, peak, Mn] = naiser_corr3(signal, Nu, N0, L, PN_seq)
    total_L = length(signal);
    preamble_L = (Nu + N0) *L;
    N_both = Nu + N0;
    Mn = [];
    num = 0;
    FACTOR = 10;
    if(total_L-preamble_L < 0)
        peak = -1;
        begin_idx = -1;
        return
    end
    for i =  1 :FACTOR: total_L-preamble_L
        seg = signal(i: i+preamble_L - 1);
        Pd = 0;
        Rd = 0;
        Rd = Rd + sum(seg( 1+N0: Nu+N0).^2);
        for k = 1: L -1
            bk = PN_seq(k)*PN_seq(k+1);
            seg1 = seg((k-1)*N_both + N0+1: (k)*N_both);
            seg2 = seg((k)*N_both + N0+1: (k+1)*N_both); 

            temp_P = bk*sum(seg1.*seg2);
            Rd = Rd + sum((seg2).^2);
            Pd = Pd + temp_P;
           
        end
        corr = Pd/Rd;
        Mn = [Mn, corr];

        num = num + 1;
    end
    
    [peak, begin_idx] = max(Mn);

    shoulder= peak*0.85;
    right = -1; 
    left = -1;
    for i = begin_idx:length(Mn)-1
        if(Mn(i) >= shoulder && Mn(i+1) <= shoulder)
            right = i;
            break;
        end
    end

    for i = begin_idx:-1:2
        if(Mn(i) >= shoulder && Mn(i-1) <= shoulder)
            left = i;
            break;
        end
    end


    if(peak > 0.35)
        if(right~=-1 && left~=-1)
            begin_idx = round(0.5*right+0.5*left)*FACTOR;
        elseif((right == -1 && left ~= -1) || (right ~= -1 && left == -1))
            begin_idx = round(begin_idx)*FACTOR;
        else
            begin_idx = -1;
        end
    else
        begin_idx = -1;
    end
    
    
    

end