close all
clear 
% clc
fs = 44100;
bias =480;
bias_shift = 240;

BW = [1000, 5000]; %[1000, 6000];
B = BW(2) - BW(1);

PN_seq = [1, 1, -1, 1];
L = length(PN_seq);
Ns = 1920; %720
GI = 540; %360
sig_len1 = Ns;
sig_len2 = (Ns + GI)*L;
file_id = '6';


save_file = strcat('N_seq5-', int2str(0), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
train_sig1= dlmread(strcat('../multi_users_FSK3/',save_file,'/train_sig1.txt'));
train_sig2 = dlmread(strcat('../multi_users_FSK3/',save_file,'/train_sig2.txt'));
train_sig2 = train_sig2(1:sig_len2);
Ns2 = length(train_sig2);
f_seq = linspace(0, fs-fs/Ns, Ns);


% exp_name = "ofdm_10";
% exp = ["1688358987728", "1684365626414"]; % "1688346034162";
% role = ["tx1", "rx1"];
% exp = ["1688359360367", "1684365998581"]; % "1688346034162";
% role = ["tx2", "rx2"];
% exp = ["1688359534071", "1684366172095"]; % "1688346034162";
% role = ["tx3", "rx3"];

% exp_name = "ofdm_20";
% exp = ["1688361291646", "1684367929790"]; % "1688346034162";
% role = ["tx1", "rx1"];
% exp = ["1688361648264", "1684368286176"]; % "1688346034162";
% role = ["tx2", "rx2"];
% exp = ["1688361992127", "1684368630094"]; % "1688346034162";
% role = ["tx3", "rx3"];

exp_name = "ofdm_28";
exp = ["1688363846460", "1684370484917"]; % "1688346034162";
role = ["tx1", "rx1"];
% exp = ["1688364188647", "1684370827101"]; % "1688346034162";
% role = ["tx3", "rx3"];
% exp = ["1688364015340", "1684370653538"]; % "1688346034162";
% role = ["tx2", "rx2"];

tx_interval1 = [];
tx_interval2 = [];

rx_interval1 = [];
rx_interval2 = [];


tap_num= Ns; %Ns
delta_f = fs / tap_num;
begin_i = round(BW(1)/delta_f) + 1;
end_i = round(BW(2)/delta_f);
f0 = (begin_i-1)*delta_f;
X = fft(train_sig1);
d = 0.2;
c = 1500;
tolerance = round(d/c*fs)+3; %4



for id =0:1
    % sending_signals = [zeros(24000,1); train_sig; zeros(24000, 1)];
    % rx_signal = awgn(sending_signals,-5,'measured');
    raw_folder = strcat('../../Range_data/state_of_art/', exp_name, '/', role(id+1), '/');
    rx_signal = dlmread(strcat(raw_folder, '/', exp(id+1), '-',file_id,'-bottom.txt'))/10000;
    rx_signal2 = dlmread(strcat(raw_folder, '/', exp(id+1), '-',file_id,'-top.txt'))/10000;

    if id == 1
        rx_signal = rx_signal(10*fs:end);
        rx_signal2 = rx_signal2(10*fs:end);
    end
    filter_order = 256;
    wn = [(BW(1) - 100)/(fs/2), (BW(2) + 200)/(fs/2)];    
    b = fir1(filter_order, wn, 'bandpass');  
    
    y_after_fir=filter(b,1,rx_signal);
    delay_fir = filter_order/2;
    rx_signal = y_after_fir(delay_fir+1:end);
    
    y_after_fir=filter(b,1,rx_signal2);
    delay_fir = filter_order/2;
    rx_signal2 = y_after_fir(delay_fir+1:end);

    
    [coarse_results] = coarse_sync_group(rx_signal, train_sig2, Ns, GI, L, PN_seq, id);
    
    figure
    hold on
    plot(rx_signal)
    for j = 1:size(coarse_results, 1)
        xline(coarse_results(j, 1)+270,  '-', "Color", "r")
        xline(coarse_results(j, 2)+270,  '--', "Color", "b")
    end
    

    fine_results = -1*ones(size(coarse_results));
    for r = 1:size(coarse_results, 1)
        if (coarse_results(r, 1) < 0 || coarse_results(r, 2) < 0)
            continue
        end
        
        f = figure('Name', int2str(100 * id + r),'visible','off');
        clf(f);
        for user_id = 1: 2
            corr_idx = coarse_results(r, user_id);    
            Hs = [];
            Hs2 = [];
            rx2 = rx_signal(corr_idx - (bias - bias_shift)+1:corr_idx+Ns2 -(bias - bias_shift));
            rx3 = rx_signal2(corr_idx - (bias - bias_shift)+1:corr_idx+Ns2 -(bias - bias_shift));
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

            if user_id == id + 1
                [midx, midx_new] = self_chirp_direct_path(h_abs,h_abs2, tolerance, bias, id);
                thesholds = [0, 0];
            else
                [midx, midx_new, thesholds] = dual_mic_direct_path(h_abs,h_abs2, tolerance, bias, 1);
            end

            fine_results(r, user_id) = (corr_idx - bias + midx_new);

            subplot(2,1, user_id)
            hold on
            plot(h_abs)
            plot(h_abs2)
            scatter(midx, h_abs(midx), 'rx')
            scatter(midx_new, h_abs2(midx_new), 'ko')
            yline(thesholds(1));
            yline(thesholds(2), '--');
            legend('bottom mic', 'top mic')

        end
        saveas(f,strcat('results/channel_', int2str(id) , '_', int2str(r), '.jpg'));
    end
    
    if id == 0
        tx_interval1 = [tx_interval1; fine_results(:, 2) - fine_results(:, 1) ];
    else
        rx_interval1 = [rx_interval1; fine_results(:, 2) - fine_results(:, 1) ];
    end

    
  
   
end
tx_num = length(tx_interval1);
rx_num = length(rx_interval1);
tx_i = 1;
rx_i  = 1;


ds_meas = [];

while 1
    if (tx_i > tx_num || rx_i > rx_num)
        break;
    end
    if(tx_interval1(tx_i) == 0 )
        tx_i = tx_i + 1; 
        continue;
    end
    if(rx_interval1(rx_i) == 0 )
        rx_i = rx_i + 1; 
        continue;
    end
    
    d = (tx_interval1(tx_i) - rx_interval1(rx_i))/2/fs*c;
    ds_meas = [ds_meas, d];

    tx_i = tx_i+1;
    rx_i = rx_i+1;
end
ds_meas
dlmwrite(strcat('../../Range_data/state_of_art/', exp_name, '/', role(1), '.txt'), ds_meas);


 
