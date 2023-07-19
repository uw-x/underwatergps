function [coarse_results] = coarse_sync_group(rx_signal, train_sig2, Ns, GI, L, PN_seq, user_id)
    %% using the cross correlation to detect preamble
    group_size = 2;
    [acor, lag] = xcorr(rx_signal, train_sig2);
    max_number = 40;
    max_peak_corr = max(acor);
    [pks,locs]=findpeaks(acor,'MinPeakHeight',max_peak_corr*0.1,'MinPeakDistance',44100*2.5);
    seek_back = 730;
    Ns2 = length(train_sig2);

    new_locs = [];
    for i = 1:length(locs)
        l= locs(i);
        p = pks(i);
        p_threshold = p*0.55; %0.65
    
        for j = seek_back : -1 : 0
            l_new = l - j;
            if(acor(l_new) > p_threshold)
                new_locs = [new_locs l_new];
                break;
            end
        end
    end
    
    begin_idx = 2-lag(1);
    acor1 = acor(begin_idx:end);
    locs0 = lag(locs);
    locs = lag(new_locs);
    
    negative_index = locs0<=0;
    locs0(negative_index) =[];
    locs(negative_index) =[];
    
    negative_index = locs<=0;
    locs0(negative_index) =[];
    locs(negative_index) =[];

    figure
    hold on
    plot(acor1)
    scatter(locs0, acor1(locs0), 'rx')
    scatter(locs, acor1(locs), 'ko')

    %% using naiser corr to exclude sparkle noise
    fs = 44100;

    variance_sample = 2000;
    look_back = 1000;
    sig_len = Ns2 + variance_sample*2;
    

    interval = round((0.75 + 0.32 )*fs);   
    coarse_results = [];
    
    
    for idx = 2:length(locs)-1
        if idx > max_number
            break
        end
        self_idx = locs(idx); % self chirp
        results_indexes = -1*ones(1, 2);
        now_offset = [];
        sigs = [];
        if user_id == 0
            tx = rx_signal(self_idx + 1-variance_sample: self_idx + sig_len);
            rx = rx_signal(self_idx + interval + 1-variance_sample: self_idx + interval + sig_len);
            sigs = [tx, rx];
            now_offset = [self_idx + 1-variance_sample, self_idx + interval + 1-variance_sample];

       else
            rx = rx_signal(self_idx -interval +1-variance_sample: self_idx -interval+ sig_len);
            tx = rx_signal(self_idx + 1-variance_sample: self_idx + sig_len);
            
            sigs = [rx, tx];
            now_offset = [self_idx - interval + 1-variance_sample, self_idx + 1-variance_sample];
       end
       
%        figure
%        subplot(211)
%        plot(sigs(:, 1))
%        subplot(212)
%        plot(sigs(:, 2))

      for sig_i = 1:2
            sig = sigs(:, sig_i);
            [naiser_idx, peak, Mn] = naiser_corr3(sig, Ns, GI, L, PN_seq);
            [acor_i, lag_i] = xcorr(sig, train_sig2);

            begin_id = floor(length(acor_i)/2);
            acor_i_seg = acor_i( begin_id + 1: end);
            if(naiser_idx > 0)
                
                [max_val, max_id] = max(acor_i_seg);
                p_threshold =max_val*0.6; %0.65
    
                for j = seek_back : -1 : 0
                    l_new = max_id - j;
                    if (l_new <= 0)
                        continue;
                    end
                    if(acor_i_seg(l_new) > p_threshold)
                        results_indexes(sig_i) = lag_i(l_new) + begin_id + now_offset(sig_i);
                        break; 
                    end
                end
            else
                break;
            end
      end
       coarse_results = [coarse_results; results_indexes];
       
    end
   

end