function [fine_result, coarse_result, rx_signal, rx_signal2] = fine_sync_recv_single(rx_signal,rx_signal2,train_sig1s, train_sig2s, fs, BW, Ns, GI, L, PN_seq, c, user_id, group_users, FSK_IDX, save_fig, N_FSK)
    d = 0.16; %% phone sieze 0.2
    tolerance = round(d/c*fs) + 3; 
    % dual mic tolerance , 3 more extra samples is for merge of peak with multipath and potch distortion
    bias = 480;
    bias_shift = 240;
    tap_num= Ns; %Ns
    Ns2 = size(train_sig2s, 1);
    have_filter = 1;

    delta_f = fs / tap_num;
    begin_i = round(BW(1)/delta_f) + 1;
    end_i = round(BW(2)/delta_f);
    f0 = (begin_i-1)*delta_f;

    %% band pass filter
    % filter the raw signal
    if have_filter
        filter_order = 256;
        wn = [(BW(1) - 100)/(fs/2), (BW(2) + 200)/(fs/2)];    
        b = fir1(filter_order, wn, 'bandpass');  
    
        y_after_fir=filter(b,1,rx_signal);
        delay_fir = filter_order/2;
        rx_signal = y_after_fir(delay_fir+1:end);
    
        y_after_fir=filter(b,1,rx_signal2);
        delay_fir = filter_order/2;
        rx_signal2 = y_after_fir(delay_fir+1:end);
    end
    
    %% apply the cross-correlation and Naiser correlation (autpo-correlation) for coarse sync
    coarse_result = coarse_sync_group(rx_signal, train_sig2s, Ns, GI, L, PN_seq, user_id, group_users, save_fig);
    fine_result = -1*ones(size(coarse_result));

    
    for r = 1:size(coarse_result, 1)
        % save the figure for the channel
        if (r < 9)
            if(save_fig)
                f =  figure('Name', int2str(200 + r),'visible','off');
            else
                f =  figure('Name', int2str(200 + r),'visible','on');
            end
            clf(f);
        end
        %if min(coarse_result(r, :), [], 'all') < 0
        % one preamble detect fail so does not go further
        if min(coarse_result(r, :)) < 0
            continue
        end
        
        %% channel estimation for fine-grained TOA estimation
        for id = 1: size(coarse_result, 2)
            user_name = group_users(id);
            if coarse_result(r, id) < 0
                continue;
            end
            train_sig1 = train_sig1s(:, id);
            X = fft(train_sig1);

            %%%%% channel estimation
            corr_idx = coarse_result(r, id);
            Hs = [];
            Hs2 = [];
            
            rx2 = rx_signal(corr_idx - (bias - bias_shift)+1:corr_idx+Ns2 -(bias - bias_shift));
            rx3 = rx_signal2(corr_idx - (bias - bias_shift)+1:corr_idx+Ns2 -(bias - bias_shift));
            
            % each preamble contains 4 OFDM
            % estimate the channel for 4 OFDM and average over OFDMs
            for i = 1: L
                begin_idx = (i-1)*(Ns + GI)+GI+1;
                end_idx = begin_idx -1 + tap_num ;
        
                Y = fft(rx2(begin_idx: end_idx) );
                Y2 = fft(rx3(begin_idx: end_idx) );
                H = complex(zeros(Ns,1));
                H2 = complex(zeros(Ns,1));
    
    
                H(begin_i:end_i) = Y(begin_i:end_i)./(PN_seq(i)*X(begin_i:end_i));
                H2(begin_i:end_i) = Y2(begin_i:end_i)./(PN_seq(i)*X(begin_i:end_i));
    
                Hs = [Hs, H];
                Hs2 = [Hs2, H2];
            end
            H = mean(Hs, 2);
            H2 = mean(Hs2, 2);
            h = ifft(H);
            h_top = ifft(H2);
            h_abs = abs(h);
            h_abs2 = abs(h_top);
            h_abs = h_abs/max(h_abs);
            h_abs2 = h_abs2/max(h_abs2);
            h_abs = [h_abs(end - bias_shift+1:end); h_abs(1:end - bias_shift)];
            h_abs2 = [h_abs2(end - bias_shift+1:end); h_abs2(1:end - bias_shift)];
            
            % h_abs and h_abs2 is the channel esitmation results 
            % for self sending preamble using self_chirp_direct_path to detect
            % path, for the recieved preamble using dual_mic_direct_path 
       
            if user_name == user_id
                [midx, midx_new] = self_chirp_direct_path(h_abs,h_abs2, tolerance, bias, user_id);
                thesholds = [0, 0];
            else
                [midx, midx_new, thesholds] = dual_mic_direct_path(h_abs,h_abs2, tolerance, bias, 1);
            end

            %%%%% seek direcrt path visualization
            if (r < 9)
                subplot(size(coarse_result, 2),1, id)
                hold on
                plot(h_abs)
                plot(h_abs2)
                scatter(midx, h_abs(midx), 'rx')
                scatter(midx_new, h_abs2(midx_new), 'ko')
%                 yline(thesholds(1));
%                 yline(thesholds(2), '--');
                legend('bottom mic', 'top mic')

            end
            fine_result(r, id) = (corr_idx - bias + midx_new);

        end
        if(save_fig)
            saveas(f,strcat('results/channel_', int2str(user_id) , '_', int2str(r), '.jpg'));
        end
    end


end