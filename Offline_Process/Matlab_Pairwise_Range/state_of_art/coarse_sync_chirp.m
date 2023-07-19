function [coarse_results, acor1] = coarse_sync_chirp(rx_signal, chirp, Ns, user_id)
    %% using the cross correlation to detect preamble
    [acor, lag] = xcorr(rx_signal, chirp);
    max_number = 40;
%   [acor, lag] = xcorr(rx_signal, train_sig2s(:, 1));
    max_peak_corr = max(acor);
    [pks,locs]=findpeaks(acor,'MinPeakHeight',max_peak_corr*0.1,'MinPeakDistance',44100*2);

    begin_idx = 2-lag(1);
    acor1 = acor(begin_idx:end);
    locs0 = lag(locs);
    
    negative_index = locs0<=0;
    locs0(negative_index) =[];
    

    figure
    hold on
    plot(acor1)
    scatter(locs0, acor1(locs0), 'rx')
    interval = round((0.75 + 0.32 )*44100);
    
    send_period = 44100*4;
    
    look_back = 1200;
    coarse_results = [];
    for idx = 2:length(locs0)-1
            if idx >= max_number
                break
            end
           self_idx = locs0(idx); % self chirp
            if idx > 1
                num_p = (self_idx -  locs0(idx-1))/send_period;
%                 num_p
            end
           %% send chirp
           if user_id == 0
                tx = rx_signal(self_idx + 1-look_back: self_idx + Ns);
                rx = rx_signal(self_idx + interval + 1-look_back: self_idx + interval + Ns);
                [acor, lag] = xcorr(rx, chirp);
                if(max(acor) > 0.04)
                    coarse_results  = [coarse_results; self_idx, self_idx+interval];
%                 else
%                     coarse_results = [coarse_results; -1, -1];
                end

           else
                rx = rx_signal(self_idx -interval +1-look_back: self_idx -interval+ Ns);
                tx = rx_signal(self_idx + 1-look_back: self_idx + Ns);
                
                [acor, lag] = xcorr(rx, chirp);
                if(max(acor) > 0.04)
                    coarse_results  = [coarse_results; self_idx-interval, self_idx];
%                 else
%                     coarse_results = [coarse_results; -1, -1];
                end
                
                
           end
            
%             figure
%             subplot(211)
%             plot(tx)
%             subplot(212)
%             plot(rx)
    end


end