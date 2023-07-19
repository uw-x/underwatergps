function [out_id, max_id] = Beepbeep(corr, Ns)
    %UNTITLED Summary of this function goes here
    %   Detailed explanation goes here
    w0 = 100;
    THMP = 0.85;
    THSD = 20;
    THVOL = 0.2;

    [peak, max_id] = max(corr);
    sharp_max = peak/mean(abs(corr(max_id - w0/2:max_id + w0/2)));
    
    look_back = 2400;
    if max_id - 2400 > 0
        begin_idx = max_id - 2400;
    else
        begin_idx = 0;
    end
    end_idx = max_id + 10;
    out_id = max_id;
    [pks,locs]=findpeaks(corr(begin_idx+1:end_idx),'MinPeakHeight',peak*THVOL,'MinPeakDistance',5);
    locs = locs + begin_idx;
%     figure
%     hold on
%     plot(abs(corr))
%     scatter(locs, corr(locs),'rx')
%     scatter(max_id, corr(max_id),'go')

    for i = 1:length(locs)
        peak_id = locs(i);
        if peak_id <= Ns/2
            continue
        end
        win_power_l1 = mean(abs(corr(peak_id - w0/2:peak_id + w0/2)));
        win_power_l2 = sumsqr(corr(peak_id - w0/2:peak_id + w0/2));
        win_power_l2_prev = sumsqr(corr(peak_id - Ns/2:peak_id - Ns/2 + w0));
        sharp = pks(i)/win_power_l1;
       
%         [i, sharp, sharp_max]
%         win_power_l2/win_power_l2_prev

        if win_power_l2/win_power_l2_prev > THSD && sharp > sharp_max*THMP
            out_id = peak_id;
            break;
        end
    end
end