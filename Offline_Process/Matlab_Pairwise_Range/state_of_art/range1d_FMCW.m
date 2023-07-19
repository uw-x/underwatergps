close all
clear 
% clc
fs = 44100;

BW = [1000, 5000]; %[1000, 6000];
B = BW(2) - BW(1);

save_file = strcat('../FMCW/');
train_sig = dlmread(strcat(save_file,'/train_sig.txt'));
Ns = length(train_sig);
f_seq = linspace(0, fs-fs/Ns, Ns);
file_id = '7';




exp_name = 'chirp_28';
% exp = {'1688364861499', '1684371499456'}; % '1688346034162';
% role = {'tx1', 'rx1'};
% sync_bias = 30000;
% exp = {'1688365074793', '1684371712616'}; % '1688346034162';
% role = {'tx2', 'rx2'};
% sync_bias = 30000;
exp = {'1688365534526', '1684372374619'}; % '1688346034162';
role = {'tx3', 'rx3'};
sync_bias = 30000;

% exp_name = 'chirp_20';
% exp = {'1688362362963', '1684369000930'}; % '1688346034162';
% role = {'tx1', 'rx1'};
% sync_bias = 30000;
% exp = {'1688362553832', '1684369192600'}; % '1688346034162';
% role = {'tx2', 'rx2'};
% sync_bias = 40000;
% exp = {'1688362730671', '1684369368797'}; % '1688346034162';
% role = {'tx3', 'rx3'};
% sync_bias = 100000;


% exp_name = 'chirp_10';
% exp = {'1688359716897', '1684366355070'}; % '1688346034162';
% role = {'tx1', 'rx1'};
% sync_bias = 1134100;
% exp = {'1688359890274', '1684366528375'}; % '1688346034162';
% role = {'tx2', 'rx2'};
% sync_bias = 1460000;
% exp = {'1688360448743', '1684367086582'}; % '1688346034162';
% role = {'tx3', 'rx3'};
% sync_bias = 747570;

tx_interval1 = [];
tx_interval2 = [];

rx_interval1 = [];
rx_interval2 = [];

NOISE_THRD = 5;

for id =0:1
    % sending_signals = [zeros(24000,1); train_sig; zeros(24000, 1)];
    % rx_signal = awgn(sending_signals,-5,'measured');
    raw_folder = strcat('../../Range_data/state_of_art/',exp_name,'/', role{id+1}, '/');
    rx_signal = dlmread(strcat(raw_folder, '/', exp{id+1}, '-',file_id,'-bottom.txt'))/10000;
    if id == 1
        rx_signal = rx_signal(10*fs+sync_bias:end);
    end
    filter_order = 256;
    wn = [(BW(1) - 100)/(fs/2), (BW(2) + 200)/(fs/2)];    
    b = fir1(filter_order, wn, 'bandpass');  
    
    y_after_fir=filter(b,1,rx_signal);
    delay_fir = filter_order/2;
    rx_signal = y_after_fir(delay_fir+1:end);
    
    figure(122)
    
    hold on
    plot(rx_signal, '--')
    
    [coarse_results, acor] = coarse_sync_chirp(rx_signal, train_sig, Ns, id);
    
    
    fine_results = [];
    for i = 1:size(coarse_results, 1)
        idx1 = coarse_results(i, 1);
        idx2 = coarse_results(i, 2);
        if idx1 < 0 || idx2 < 0
            fine_results = [fine_results; -1, -1];
            continue
        end
        cor1 = acor(idx1 - 2*Ns+1:idx1 + Ns);
        cor2 = acor(idx2 - 2*Ns+1:idx2 + Ns);
    
        [out_id1, max_id1] = Beepbeep(cor1, Ns);
        out_id1 = out_id1 + (idx1 - 2*Ns);
        max_id1 = max_id1 + (idx1 - 2*Ns);
        [out_id2, max_id2] = Beepbeep(cor2, Ns);
        out_id2 = out_id2 + (idx2 - 2*Ns);
        max_id2 = max_id2 + (idx2 - 2*Ns);
        if id == 0
            fine_results = [fine_results; max_id1, out_id2];
        else
            fine_results = [fine_results; out_id1, max_id2];
        end
            
        
    end
    figure
    hold on
    plot(acor)
    for i = 1:size(fine_results, 1)
        
        id1 = fine_results(i, 1);
        id2 = fine_results(i, 2);
        if id1 < 0 || id2 < 0
            continue
        end
        scatter(id1, acor(id1), 'rx')
        scatter(id2, acor(id2), 'ko')
    end
    
    
    
    if id == 0
        tx_interval1 = fine_results;
%         tx_interval1 = [tx_interval1; fine_results(:, 2) - fine_results(:, 1) ];
    else
        rx_interval1 = fine_results;
%         rx_interval1 = [rx_interval1; fine_results(:, 2) - fine_results(:, 1) ];
    end
    % %% auto correlation
    % [acor, lag] = xcorr(rx_signal, train_sig);
    % max_peak_corr = max(acor);
    % [pks,locs]=findpeaks(acor,'MinPeakHeight',max_peak_corr*0.1,'MinPeakDistance',fs*3);
    % 
    % 
    % [out_id, max_id] = Beepbeep(acor, Ns);
    % figure
    % hold on
    % plot(lag, acor)
    % scatter(lag(max_id), acor(max_id), 'rx')
    % scatter(lag(out_id), acor(out_id), 'ko')
    
    
    % seek_back = 730;
    % new_locs = [];
    % for i = 1:length(locs)
    %     l= locs(i);
    %     p = pks(i);
    %     p_threshold = p*0.55; %0.65
    %     for j = seek_back : -1 : 0
    %         l_new = l - j;
    %         if(acor(l_new) > p_threshold)
    %             new_locs = [new_locs l_new];
    %             break;
    %         end
    %     end
    % end
    % 
    %     
    % begin_idx = 2-lag(1);
    % acor1 = acor(begin_idx:end);
    % locs0 = lag(locs);
    % locs = lag(new_locs);
    % 
    % negative_index = locs0<=0;
    % locs0(negative_index) =[];
    % locs(negative_index) =[];
    % 
    % negative_index = locs<=0;
    % locs0(negative_index) =[];
    % locs(negative_index) =[];
    
    
    %% FMCW 
    MAX_SHIFT = 8000;
    lookback = 1800;
    
    f_max = MAX_SHIFT/Ns*B;
    index_max = ceil(f_max/(fs/Ns));
    reso = 1/fs*B;
    
    
    fine_results2 = [];
    for i = 1:size(coarse_results, 1)
        idx1 = coarse_results(i, 1) - lookback;
        idx2 = coarse_results(i, 2) - lookback;
        if idx1 < 0 || idx2 < 0
            fine_results2 = [fine_results2; -1, -1];
            continue
        end
        recv_sig1 = rx_signal(idx1 + 1 : idx1+Ns); 
        recv_sig2 = rx_signal(idx2 + 1 : idx2+Ns); 
        
        demod = abs(fft(recv_sig1.*train_sig));
        valid_period = demod(1:index_max);
        noise_period = mean(demod(index_max + 800 : index_max+ 1800 )) * NOISE_THRD;
        [peak_max,idx] = max(valid_period);
        noise_period = max([noise_period, peak_max*0.2]);
        if id == 0
            f_direct=f_seq(idx);
            dst_id = idx;
            R1 = (f_direct/B)*Ns + idx1;
        else
            [pks,locs]=findpeaks(valid_period,'MinPeakHeight',noise_period,'MinPeakDistance',2);
            if length(locs) == 0
                dst_id = idx;
                f_direct =f_seq(idx);
            else
                f_direct =f_seq(locs(1));
                dst_id = locs(1);
            end
            R1 = (f_direct/B)*Ns + idx1;
        end
    
%         figure
%         hold on
%         plot(f_seq, demod)
%         plot([0, fs], [noise_period, noise_period], 'k--')
% %         scatter(f_seq(locs), demod(locs), 'rx')
%         scatter(f_seq(dst_id), demod(dst_id), 'bo')
    
    
        demod = abs(fft(recv_sig2.*train_sig));
        valid_period = demod(1:index_max);
        noise_period = mean(demod(index_max + 800 : index_max+ 1800 )) * NOISE_THRD;
        [peak_max,idx] = max(valid_period);
        noise_period = max([noise_period, peak_max*0.2]);
        if id == 0
            [pks,locs]=findpeaks(valid_period,'MinPeakHeight',noise_period,'MinPeakDistance',2);
            f_direct =f_seq(locs(1));
            dst_id = locs(1);
            R2 = (f_direct/B)*Ns + idx2;
        else
            if length(locs) == 0
                dst_id = idx;
                f_direct =f_seq(idx);
            else
                f_direct =f_seq(locs(1));
                dst_id = locs(1);
            end
            R2 = (f_direct/B)*Ns + idx2;
        end
    
    
%         figure
%         hold on
%         plot(f_seq, demod)
%         plot([0, fs], [noise_period, noise_period], 'k--')
% %         scatter(f_seq(locs), demod(locs), 'rx')
%         scatter(f_seq(dst_id), demod(dst_id), 'bo')
    
        fine_results2 = [fine_results2; R1, R2];
        
    end
    
    if id == 0
        tx_interval2 = fine_results2;%[tx_interval2; fine_results2(:, 2) - fine_results2(:, 1) ];
    else
        rx_interval2 = fine_results2; % [rx_interval2; fine_results2(:, 2) - fine_results2(:, 1) ];
    end
end


tx_num = size(tx_interval1, 1);
rx_num = size(rx_interval1, 1);

% tx_interval1 = tx_interval1 - tx_interval1(1,1);
% rx_interval1 = rx_interval1 - rx_interval1(1,1);
% tx_interval2 = tx_interval2 - tx_interval2(1,1);
% rx_interval2 = rx_interval2 - rx_interval2(1,1);

tx_i = 1;
rx_i = 1;

dis_meas = [];
while 1
    if(tx_i > tx_num || rx_i > rx_num)
        break;
    end
    
    if abs(tx_interval1(tx_i, 1) - rx_interval1(rx_i, 1)) <= fs
        d1 = ( (tx_interval1(tx_i, 2) - tx_interval1(tx_i, 1)) - (rx_interval1(rx_i, 2) - rx_interval1(rx_i, 1)) )/fs/2*1500;
        d2 = ( (tx_interval2(tx_i, 2) - tx_interval2(tx_i, 1)) - (rx_interval2(rx_i, 2) - rx_interval2(rx_i, 1)) )/fs/2*1500;
        tx_i = tx_i + 1;
        rx_i = rx_i + 1;
        dis_meas = [dis_meas; d1, d2];
    elseif(tx_interval1(tx_i, 1) - rx_interval1(rx_i, 1) > fs)
        rx_i = rx_i + 1;
    elseif(rx_interval1(rx_i, 1) - tx_interval1(tx_i, 1) > fs)
        tx_i = tx_i + 1;
    end
    
end

dis_meas
dlmwrite(strcat('../../Range_data/state_of_art/', exp_name, '/', role{1}, '.txt'), dis_meas);
% (tx_interval1 - rx_interval1)/fs/2*c
% (tx_interval2 - rx_interval2)/fs/2*c






 