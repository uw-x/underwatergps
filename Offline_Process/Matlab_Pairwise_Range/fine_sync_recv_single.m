function [fine_result, coarse_result, rx_signal, rx_signal2] = fine_sync_recv_single(rx_signal,rx_signal2,train_sig1s, train_sig2s, fs, BW, Ns, GI, L, PN_seq, c, user_id, group_users, FSK_IDX, save_fig, N_FSK)
    d = 0.2;
    group_size = length(group_users);
    position = [0, 0; d, 0];
    bias =480;
    bias_shift = 240;
    tap_num= Ns; %Ns
    Ns2 = size(train_sig2s, 1);
    have_filter = 1;

    delta_f = fs / tap_num;
    begin_i = round(BW(1)/delta_f) + 1;
    end_i = round(BW(2)/delta_f);
    f0 = (begin_i-1)*delta_f;

    %% band pass filter
    if have_filter
        %% begin to process
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

%     figure
%     plot(rx_signal)
%      audiowrite('save.wav',rx_signal/max(abs(rx_signal)),44100);
%     rx_signal = rx_signal(1877000+1 : 1877000+fs*6);
%     [naiser_idx, peak, Mn] = naiser_corr3(rx_signal, Ns, GI, L, PN_seq);
%     figure
%     subplot(211)
%     plot(rx_signal)
%     subplot(212)
%     hold on
%     x = (1:length(Mn))*16;
%     plot(x, Mn)
%     ylim([-0.5,1])

    coarse_result = coarse_sync_group(rx_signal, train_sig2s, Ns, GI, L, PN_seq, user_id, group_users, save_fig);
    fine_result = -1*ones(size(coarse_result));


    global_idx = [];
    tolerance = round(d/c*fs)+3; %4

    for r = 1:size(coarse_result, 1)
        if (r < 9)
            if(save_fig)
                f =  figure('Name', int2str(200 + r),'visible','off');
            else
                f =  figure('Name', int2str(200 + r),'visible','on');
            end
            clf(f);
            
        end
        if min(coarse_result(r, :), [], 'all') < 0
            continue
        end
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
%             if id > 2
%                 top = rx_signal(corr_idx - (bias - bias_shift)+1:corr_idx+Ns2 -(bias - bias_shift));
%                 bottom = rx_signal2(corr_idx - (bias - bias_shift)+1:corr_idx+Ns2 -(bias - bias_shift));
%                 figure
%                 hold on
%                 plot(top)
%                 plot(bottom, '--')
%                 [acor, lag] = xcorr(bottom, top);
%                 figure
%                 plot(lag, acor)
%                 xlim([-14, 14])
%             end

            rx2 = rx_signal(corr_idx - (bias - bias_shift)+1:corr_idx+Ns2 -(bias - bias_shift));
            rx3 = rx_signal2(corr_idx - (bias - bias_shift)+1:corr_idx+Ns2 -(bias - bias_shift));

            user_sig = rx_signal(corr_idx + Ns2+1  + GI + 150 : corr_idx + Ns2+ N_FSK + GI+150);
%             figure
%             plot(rx_signal2(corr_idx - bias+1:corr_idx+Ns2 +tap_num + GI-bias));

%             user_id0 = check_FSK(user_sig, FSK_IDX)-1;
%             if(user_id0 ~= user_name)
%                 [user_id0, user_name]
%             end

%             assert(user_id0 == id)
%             user_id = check_DIFF(user_sig1, user_sig2, train_sig1, FSK_IDX);
%             figure
%             subplot(211)
%             plot(rx2)
%             subplot(212)
%             plot(rx3)
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
%             close all
%             figure(300)
%             hold on
%             for t = 1:size(Hs,2)
%                 
%                 plot(real(ifft(Hs(:, t))))
%             end

            h = ifft(H);
            h_top = ifft(H2);
            h_abs = abs(h);
            h_abs2 = abs(h_top);
            h_real = real(h);
            h_real2 = real(h_top);
%             figure
%             plot(h_abs)

            h_abs = h_abs/max(h_abs);
            h_abs2 = h_abs2/max(h_abs2);
            h_real = h_real/max(h_real);
            h_real2 = h_real2/max(h_real2);

            h_abs = [h_abs(end - bias_shift+1:end); h_abs(1:end - bias_shift)];
            h_abs2 = [h_abs2(end - bias_shift+1:end); h_abs2(1:end - bias_shift)];
            h_real = [h_real(end - bias_shift+1:end); h_real(1:end - bias_shift)];
            h_real2 = [h_real2(end - bias_shift+1:end); h_real2(1:end - bias_shift)];

            if user_name == user_id
                [midx, midx_new] = self_chirp_direct_path(h_abs,h_abs2, tolerance, bias, user_id);
                thesholds = [0, 0];
            else
                [midx, midx_new, thesholds] = dual_mic_direct_path(h_abs,h_abs2, tolerance, bias, 1);
            end

            %%%%% seek direcrt path
           
            if (r < 9)
                
                subplot(size(coarse_result, 2),1, id)
                hold on
                plot(h_abs)
                plot(h_abs2)
                scatter(midx, h_abs(midx), 'rx')
                scatter(midx_new, h_abs2(midx_new), 'ko')
                yline(thesholds(1));
                yline(thesholds(2), '--');
                legend('bottom mic', 'top mic')

            end
            fine_result(r, id) = (corr_idx - bias + midx_new);

        end
        if(save_fig)
            saveas(f,strcat('results/channel_', int2str(user_id) , '_', int2str(r), '.jpg'));
        end
    end


end