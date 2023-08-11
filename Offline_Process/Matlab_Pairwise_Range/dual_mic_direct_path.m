function [midx, midx_new, thesholds, which_channel] = dual_mic_direct_path(h_abs,h_abs2, tolerance, bias, noise_adapt)
%UNTITLED2 Summary of this function goes here
    if(noise_adapt == 0)
        %% using fixed noise threshold
        threshold = 0.35;
        low_threshold = threshold - 0.02;
        threshold2 = threshold;
        low_threshold2 = threshold2 - 0.02;
    else
        %% using adaptive noise threshold based on channel estimation
        noise_level1 = mean(abs(h_abs(end - 600:end - 100)));
        noise_level2 = mean(abs(h_abs2(end - 600:end - 100)));
        threshold =  max([0.32, noise_level1 + 0.2]);
        low_threshold =threshold - 0.02;
        threshold2 = max([0.32, noise_level2 + 0.2]);
        low_threshold2 = threshold2 - 0.02;
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
    

    %% find the peak of estimated channel and aet them as the direct path candidates
    [h_pks1,h_locs1]=findpeaks(h_seg,'MinPeakHeight',h_peak*threshold,'MinPeakDistance',2);
    [h_pks2,h_locs2]=findpeaks(h_seg2,'MinPeakHeight',h_peak*threshold2,'MinPeakDistance',2);

    midx_new = midx;
    which_channel = 1;
    if(isempty(h_locs2) || isempty(h_locs1))
        % if one of the channel has no peak do not apply the dual-channel
        % algorithm
        midx_new = midx;
    else   
        %% dual-channel peak detection, here, we try to find the first apparent peak appear in both channel
        %%% the implementation begins with the first sample and iterates
        %%% until find the first valid peak
        i1 = 1;
        i2 = 1;
        for ii = 1:(length(h_locs1) + length(h_locs2))
            
            %%% check the termination condition, when one channel arrives
            %%% at the last peak
            if(i1 > length(h_locs1))
                l1 = 99999;
            else
                l1 = h_locs1(i1);
            end
            if(i2 > length(h_locs2))
                l2 = 99999;
            else
                l2 = h_locs2(i2);
            end
            
            %%% tolerance is the physic constrains between 2 channels
            if(l1 < l2 )

                %%% this part just avoids index out of boundary
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
                %%%

                if(max(seg) > low_threshold2 && l1 > midx - bias - 500) 
                    %%% find the valid direct path and break
                    midx_new = l1;
                    which_channel = 1;
                    break;
                else
                    %%% not find the valid direct path and continue to seek
                    i1 = i1+1;
                end
            else
                %%% this part just avoids index out of boundary
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
                    %%% find the valid direct path and break
                    midx_new = l2;
                    which_channel = 2;
                    break;
                else
                    %%% not find the valid direct path and continue to seek
                    i2 = i2+1;
                end
            end
        end
    end
    assert(midx_new > 0);
end