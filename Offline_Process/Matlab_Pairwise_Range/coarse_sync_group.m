function [coarse_results] = coarse_sync_group(rx_signal, train_sig2s, Ns, GI, L, PN_seq, user_id, group_users, save_fig)
    group_size = length(group_users);
    %% using the cross correlation to detect preamble
    [acor, lag] = xcorr(rx_signal, train_sig2s(:, user_id + 1));
    max_number = 30;
    max_peak_corr = max(acor);
    [pks,locs]=findpeaks(acor,'MinPeakHeight',max_peak_corr*0.1,'MinPeakDistance',44100*4);
    seek_back = 730;
    Ns2 = length(train_sig2s(:, user_id + 1 ));

    new_locs = [];
    % look back to find the previous high peak instead of highest peak in consideration of multipath
    for i = 1:length(locs)
        l= locs(i);
        p = pks(i);
        p_threshold = p*0.55;
    
        for j = seek_back : -1 : 0
            l_new = l - j;
            if(acor(l_new) > p_threshold)
                new_locs = [new_locs l_new];
                break;
            end
        end
    end
    
    locs = lag(new_locs);
    negative_index = locs<=0;
    locs(negative_index) =[];
    
    %% using naiser corr to exclude sparkle noise
    interval = 15876; % interval between the reply of each users 
    reply_interval = 37000; % interval between the the leader signal and first user signals
    sig_len = Ns2 + 4000;
        
    coarse_results = [];
    
    
    %%% find the timing the sending sigals and the receiving signals of 
    offsets_all = [0, reply_interval];
    offsets = [];
    user_idx = -1;
    for i =3:12
        offsets_all = [offsets_all, reply_interval+(i-2)*interval];
    end
    for u = 1:length(group_users)
        id = group_users(u);
        if(id == user_id)
            user_idx = u;
        end
        offsets = [offsets, offsets_all(id+1)];
    end
    
  
    for idx = 2:length(locs)
        if idx > max_number
            break
        end
        self_idx = locs(idx); % self chirp

        begin_indexes = zeros(1, group_size);
        results_indexes = -1*ones(1, group_size);
        for chirp_id = 1:group_size
            begin_indexes(chirp_id) = self_idx - offsets(user_idx) + offsets(chirp_id) - 800;
        end 
        
        %%% begin_indexes is the begin index for each preamble including
        %%% sending and recieving preamble
        if(idx == 4)
            f = figure('Name', int2str(100 + idx),'visible','on');
            clf(f);
        end
        
        %%% checking whether each preamble is valid using naiser correlation
        miss_leader = 0;
        num = 0;
        for begin_id = begin_indexes
            num = num + 1;
            
            %%% chunk the preamble
            if(miss_leader == 0 || num == user_id + 1)
                if begin_id + sig_len < length(rx_signal) && begin_id > 0
                    rx1 = rx_signal(begin_id+1:begin_id+sig_len);
                elseif begin_id <= 0
                    begin_indexes(num) = 1;
                    rx1 = rx_signal(1:begin_id+sig_len);
                else
                    rx1 = rx_signal(begin_id + 1:end);
                end
            else
                rx1 = rx_signal(begin_id+1 - interval*5 :begin_id+sig_len- interval*5);
            end
            
            %%% apply the naiser on the current chunked preamble
            [naiser_idx, peak, Mn] = naiser_corr3(rx1, Ns, GI, L, PN_seq); 
            
            
            %%% apply the naiser on the current nearby preambles to see
            %%% whether missing any other preambles
            if(user_id == 0)
                if(naiser_idx < 0)
                    rx1 =  rx_signal(begin_id+1+interval*5:begin_id+sig_len+interval*5);
                    [naiser_idx, peak, Mn] = naiser_corr3(rx1, Ns, GI, L, PN_seq);
                    now_offset =  begin_indexes(num) + interval*5;
                else
                    now_offset =  begin_indexes(num);
                end
            else
                if(num == 1 && naiser_idx < 0)
                    rx1 =  rx_signal(begin_id+1-interval*5:begin_id+sig_len-interval*5);
                    [naiser_idx, peak, Mn] = naiser_corr3(rx1, Ns, GI, L, PN_seq);
                    now_offset =  begin_indexes(num) - interval*5;
                    miss_leader = 1;
                elseif(miss_leader == 1 && num ~= user_id + 1)
                    now_offset =  begin_indexes(num) - interval*5;
                else
                    now_offset =  begin_indexes(num);
                end
            end
            
            
            %%% now the preamble is valid and we use cross correlation to determine the coarse index 
            [acor_i, lag_i] = xcorr(rx1, train_sig2s(:, num));
            begin_id = floor(length(acor_i)/2);
            acor_i_seg = acor_i( begin_id + 1: end);
            if(naiser_idx > 0)
                [max_val, max_id] = max(acor_i_seg);
                p_threshold =max_val*0.6; 
                for j = seek_back : -1 : 0
                    l_new = max_id - j;
                    if (l_new <= 0)
                        continue;
                    end
                    if(acor_i_seg(l_new) > p_threshold)
                        results_indexes(num) = lag_i(l_new) + begin_id + now_offset;
                        break; 
                    end
                end

            end
            
            %%% visulization
            if(idx == 4 )
                subplot(group_size, 3, 3*num - 2)
                plot(rx1)
                if(num == 1) 
                    title('Time Domain signal'); 
                end
                subplot(group_size, 3, 3*num - 1)
                plot(acor_i_seg)
                if(num == 1) 
                    title('Cross-correlation'); 
                end
                xline(results_indexes(num) - begin_indexes(num) )
                subplot(group_size, 3, 3*num)
                plot(Mn)
                if(num == 1) 
                    title('Naiser Cross-correlation'); 
                end
                ylim([-0.3,1])
                xlim([0, 600])
                
            end


        end
        coarse_results = [coarse_results; results_indexes];
       
    end
   

end