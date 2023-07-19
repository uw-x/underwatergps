function [midx, midx_new, thesholds, which_channel] = dual_mic_direct_path(h_abs,h_abs2, tolerance, bias, noise_adapt)
%UNTITLED2 Summary of this function goes here
%   Detailed explanation goes here\
    if(noise_adapt == 0)
        threshold = 0.35;
        low_threshold = threshold - 0.02;
        threshold2 = threshold;
        low_threshold2 = threshold2 - 0.02;
    else
        noise_level1 = mean(abs(h_abs(end - 600:end - 100)));
        noise_level2 = mean(abs(h_abs2(end - 600:end - 100)));
        threshold =  max([0.3, noise_level1 + 0.24]);
        low_threshold =threshold ;%- 0.02;
        threshold2 = max([0.3, noise_level2 + 0.24]);
        low_threshold2 = threshold2;% - 0.02;
    end
    thesholds = [threshold, threshold2] ;
    [h_peak,midx]=max(h_abs);
    if (midx+tolerance+20 > length(h_abs))
        h_seg = h_abs;
    else
        h_seg = h_abs(1:midx+tolerance+20);
    end
    if (midx+tolerance+20 > length(h_abs2))
        h_seg2 = h_abs2; 
    else
        h_seg2 = h_abs2(1:midx+tolerance+20);
    end   

    [h_pks1,h_locs1]=findpeaks(h_seg,'MinPeakHeight',h_peak*threshold,'MinPeakDistance',2);
    [h_pks2,h_locs2]=findpeaks(h_seg2,'MinPeakHeight',h_peak*threshold2,'MinPeakDistance',2);
%                 figure
%                 hold on
%                 plot(h_seg)
%                 scatter(h_locs1, h_pks1);
%                 plot(h_seg2)
%                 scatter(h_locs2, h_pks2);
    midx_new = midx;
    which_channel = 1;
    if(isempty(h_locs2) || isempty(h_locs1))
        midx_new = midx;
    else        
        i1 = 1;
        i2 = 1;
        for ii = 1:(length(h_locs1) + length(h_locs2))
            if(i1 > length(h_locs1))
                l1 = 99999;%h_locs1(end);
            else
                l1 = h_locs1(i1);
            end
            if(i2 > length(h_locs2))
                l2 = 99999;%h_locs2(end);
            else
                l2 = h_locs2(i2);
            end
    
            if(l1 < l2 )
                if l1 - tolerance < 1
                    begin_idx = 1;
                else
                    begin_idx = l1 - tolerance;
                end
                if l1 + tolerance > length(h_seg2)
                    end_idx = length(h_seg2);
                else
                    end_idx = l1 + tolerance;
                end            
                seg = h_seg2(begin_idx:end_idx);
                
                if(max(seg) > low_threshold2 && l1 > midx - bias - 500)
%                 if( (max(seg) > low_threshold2 || h_seg(l1) > 0.81 ) && l1 > midx - bias - 500)
                    midx_new = l1;
                    which_channel = 1;
                    break;
                else
                    i1 = i1+1;
                end
            else
                if l2 - tolerance < 1
                    begin_idx = 1;
                else
                    begin_idx = l2 - tolerance;
                end
                if l2 + tolerance > length(h_seg2)
                    end_idx = length(h_seg2);
                else
                    end_idx = l2 + tolerance;
                end            
                seg = h_seg(begin_idx:end_idx);
                
                if(max(seg) > low_threshold && l2 > midx - bias -500)
%                 if( (max(seg) > low_threshold || h_seg2(l2) > 0.81 ) && l2 > midx - bias -500)
                    midx_new = l2;
                    which_channel = 2;
                    break;
                else
                    i2 = i2+1;
                end
            end
        end
    end
    assert(midx_new > 0);
end