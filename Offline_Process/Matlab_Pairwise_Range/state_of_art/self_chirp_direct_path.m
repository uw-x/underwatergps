function [midx, midx_new] = self_chirp_direct_path(h_abs,h_abs2, tolerance, bias, user_id)
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here   
    threshold = 0.35;
    threshold2 = 0.35;
    
    [h_peak,midx]=max(h_abs);
    [h_peak2,midx2]=max(h_abs2);
    midx_new = min([midx, midx2]);
%     if(user_id > 0)
%         if (midx+tolerance+20 > length(h_abs))
%             h_seg = h_abs;
%         else
%             h_seg = h_abs(1:midx+tolerance+20);
%         end
%         [h_pks1,h_locs1]=findpeaks(h_seg,'MinPeakHeight',h_peak*0.75,'MinPeakDistance',2);
%         midx_new = midx;
%         for i = 1:length(h_locs1)
%             l = h_locs1(i);
%             if(midx - l > bias + 300) 
%                 continue;
%             else
%                 midx_new = l;
%                 break;
%             end
%         end
%     else
% %         midx_new = floor((midx + midx2)/2);
%         midx_new = min([midx, midx2]);
%     end

end